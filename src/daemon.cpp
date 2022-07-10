#include "daemon.hpp"

#include <algorithm>
#include <arpa/inet.h>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <linux/close_range.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/capability.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/syscall.h>

#include "bytemanip.hpp"
#include "datagram.hpp"
#include "log/logger.hpp"
#include "log/syslog_logsink.hpp"
#ifdef HAVE_SYSTEMD
#include "log/systemd_logsink.hpp"
#endif
#include "src/socket.hpp"
#include "string-format.hpp"

namespace tinydhcpd {
volatile std::sig_atomic_t last_signal;

constexpr uint8_t DHCP_TYPE_DISCOVER = 1;
constexpr uint8_t DHCP_TYPE_OFFER = 2;
constexpr uint8_t DHCP_TYPE_REQUEST = 3;
constexpr uint8_t DHCP_TYPE_DECLINE = 4;
constexpr uint8_t DHCP_TYPE_ACK = 5;
constexpr uint8_t DHCP_TYPE_NAK = 6;
constexpr uint8_t DHCP_TYPE_RELEASE = 7;
constexpr uint8_t DHCP_TYPE_INFORM = 8;

constexpr uint16_t DHCP_CLIENT_PORT = 68;

const std::string LEASE_FILE_DELIMITER = ",";

std::unique_ptr<tinydhcpd::Logger> global_logger;

void sighandler(int signum) {
  tinydhcpd::LOG_TRACE("Caught signal " + std::to_string(signum));
  tinydhcpd::last_signal = signum;
}

Daemon::Daemon(const struct in_addr &address, const std::string &iface_name,
               SubnetConfiguration &netconfig,
               const std::string &lease_file_path) try
    : socket(address, iface_name, *this),
      epoll_socket(socket, (EPOLLIN | EPOLLOUT)), netconfig(netconfig),
      lease_file_path(lease_file_path), active_leases() {
  load_leases();
} catch (std::runtime_error &ex) {
  printf("%s\n", ex.what());
  std::exit(EXIT_FAILURE);
}

void Daemon::daemonize(const DAEMON_TYPE type) {
  if (type == DAEMON_TYPE::SYSV) {
    // 1. close all file descriptors besides std{in, out, err}
    struct rlimit res_lim;
    getrlimit(RLIMIT_NOFILE, &res_lim);
    if (
#ifdef __MUSL__
        syscall(SYS_close_range, 3, res_lim.rlim_cur, CLOSE_RANGE_CLOEXEC)
#else
        close_range(3, res_lim.rlim_cur, CLOSE_RANGE_CLOEXEC)
#endif
        != 0) {
      throw std::runtime_error(string_format(
          "Failed to close file descriptors: %s", strerror(errno)));
    }
    // 2. reset all signal handlers
    struct sigaction default_handler;
    default_handler.sa_handler = SIG_DFL;
    for (int i = 0; i < _NSIG; i++) {
      sigaction(i, &default_handler, nullptr);
    }
    // 3. reset sig mask
    sigset_t set;
    sigemptyset(&set);
    if (sigprocmask(SIG_SETMASK, &set, nullptr) < 0) {
      throw std::runtime_error(
          string_format("sigprocmask failed! Error %s", strerror(errno)));
    }
    // 4. reset env variables (none set)
    // 5. do the forking
    int pipefd[2];
    pipe(pipefd);
    pid_t child = fork();
    if (child < 0) {
      throw std::runtime_error(
          string_format("Failed to fork! Error: %s", strerror(errno)));
    } else if (child == 0) {
      close(pipefd[0]);
      uint8_t pipe_msgbuf[1] = {0};
      // 6. start a new session to detach from terminals
      setsid();
      // 7. fork some more to prevent terminal re-attachment by accident
      child = fork();
      if (child < 0) {
        write(pipefd[1], pipe_msgbuf, 1);
        throw std::runtime_error(
            string_format("Failed to fork! Error: %s", strerror(errno)));
      } else if (child != 0) {
        // 8. finish first child process
        std::exit(EXIT_SUCCESS);
      }
      // now we're in the actual daemon, set logger accordingly
      tinydhcpd::global_logger.reset(
          new tinydhcpd::Logger(*(new tinydhcpd::SyslogLogSink())));
      // if (getppid() != 1) {
      //   write(pipefd[1], pipe_msgbuf, 1);
      //   throw std::runtime_error(
      //       "The daemon has not been reparented to PID 1!");
      // }
      //  9. connect std{in, out, err} to /dev/null
      std::freopen("/dev/null", "r", stdin);
      std::freopen("/dev/null", "w", stdout);
      std::freopen("/dev/null", "w", stderr);
      // 10. set umask to 0
      umask(0);
      // 11. cd to root to avoid accidental unmount blocking
      std::filesystem::current_path("/");
      // 12. write PID to file
      const std::string pid_file_path = "/run/tinydhcpd.pid";
      std::ofstream pid_file(pid_file_path, std::ios::trunc);
      pid_file << getpid() << std::endl;
      pid_file.close();
      // 13. drop capabilities if necessary
      const cap_t caps = cap_get_proc();
      cap_flag_value_t flag_value;
      const cap_value_t cap_array[] = {CAP_NET_ADMIN};
      if (cap_get_flag(caps, cap_array[0], CAP_EFFECTIVE, &flag_value) < 0) {
        LOG_WARN("Failed to get info about capability CAP_NET_ADMIN");
      }
      if (flag_value != CAP_SET) {
        // assume process was started as root
        cap_set_flag(caps, CAP_EFFECTIVE, 1, cap_array, CAP_SET);
        if (cap_set_proc(caps) < 0) {
          write(pipefd[1], pipe_msgbuf, 1);
          throw std::runtime_error("Failed to get CAP_NET_ADMIN!");
        }
      }
      if (getuid() == 0) {
        setuid(300);
      }
      if (getgid() == 0) {
        setgid(300);
      }
      // tell the parent that everything is OK
      pipe_msgbuf[0] = 1;
      write(pipefd[1], pipe_msgbuf, 1);
      close(pipefd[1]);
    } else {
      close(pipefd[1]);
      uint8_t pipe_recvbuf[1] = {0};
      read(pipefd[0], pipe_recvbuf, 1);
      if (pipe_recvbuf[0] == 1) {
        std::exit(EXIT_SUCCESS);
      } else {
        LOG_ERROR("Error in child process!");
        std::exit(EXIT_FAILURE);
      }
    }
  }
#ifdef HAVE_SYSTEMD
  else {
    tinydhcpd::global_logger.reset(
        new tinydhcpd::Logger(*(new tinydhcpd::SystemdLogSink(std::cerr))));
  }
#endif
}

void Daemon::main_loop() { epoll_socket.poll_loop(); }

void Daemon::handle_recv(DhcpDatagram &datagram) {
  std::ostringstream os;
  os << "Received packet from ";
  for (int i = 0; i < datagram.hwaddr_len; i++) {
    os << string_format("%x", datagram.hw_addr[i]) << ":";
  }
  LOG_DEBUG(os.str());
  LOG_TRACE(string_format("XID: %#010x", datagram.transaction_id));
  if (datagram.opcode != 0x1) {
    return;
  }
  std::for_each(datagram.options.begin(), datagram.options.end(),
                [](const auto &option) {
                  std::ostringstream os;
                  os << string_format("Tag %u | Length %u | Value(s) ",
                                      static_cast<uint8_t>(option.first),
                                      option.second.size());
                  for (uint8_t val_byte : option.second) {
                    os << string_format("%#04x ", val_byte);
                  }
                  LOG_TRACE(os.str());
                });

  switch (datagram.options[OptionTag::DHCP_MESSAGE_TYPE].at(0)) {
  case DHCP_TYPE_DISCOVER:
    LOG_DEBUG("DISCOVER");
    handle_discovery(datagram);
    break;
  case DHCP_TYPE_REQUEST:
    LOG_DEBUG("REQUEST");
    handle_request(datagram);
    break;
  case DHCP_TYPE_RELEASE:
    LOG_DEBUG("RELEASE");
    handle_release(datagram);
    break;
  case DHCP_TYPE_INFORM:
    LOG_DEBUG("INFORM");
    handle_inform(datagram);
    break;
  case DHCP_TYPE_DECLINE:
    LOG_DEBUG("DECLINE");
    handle_decline(datagram);
    break;
  default:
    LOG_WARN(
        string_format("Invalid message type: %x",
                      datagram.options[OptionTag::DHCP_MESSAGE_TYPE].at(0)));
  }
}

void Daemon::handle_discovery(const DhcpDatagram &datagram) {
  DhcpDatagram reply = create_skeleton_reply_datagram(datagram);
  struct ether_addr request_hwaddr {};
  std::copy(datagram.hw_addr.cbegin(),
            datagram.hw_addr.cbegin() + datagram.hwaddr_len,
            request_hwaddr.ether_addr_octet);
  in_addr_t offer_address_host_order = datagram.client_ip;
  in_addr_t range_start_host_order = ntohl(netconfig.range_start.s_addr);
  in_addr_t range_end_host_order = ntohl(netconfig.range_end.s_addr);

  // figure out what address we can give the client
  update_leases();
  if (datagram.options.contains(OptionTag::REQUESTED_IP_ADDRESS)) {
    in_addr_t requested_ip = to_number<in_addr_t>(
        datagram.options.at(OptionTag::REQUESTED_IP_ADDRESS));
    if (requested_ip >= range_start_host_order &&
        requested_ip <= range_end_host_order) {
      if (active_leases.contains(datagram.hw_addr) &&
          active_leases.at(datagram.hw_addr).first == requested_ip) {
        offer_address_host_order = requested_ip;
      }
    }
  } else if (offer_address_host_order != INADDR_ANY) {
    // the client has not requested a specific lease, so we just return the
    // remaining lease time
    uint64_t now = get_current_time();
    uint32_t remaining =
        static_cast<uint32_t>(active_leases.at(datagram.hw_addr).second - now);
    reply.options[OptionTag::LEASE_TIME] = to_byte_vector(remaining);
  }
  if (offer_address_host_order == INADDR_ANY) {
    if (!netconfig.fixed_hosts.contains(request_hwaddr)) {
      offer_address_host_order = range_start_host_order;
      for (auto &entry : active_leases) {
        if (offer_address_host_order == entry.second.first) {
          offer_address_host_order++;
        }
        if (offer_address_host_order >= range_end_host_order) {
          LOG_ERROR("Failed to find free address!");
          return;
        } else {
          break;
        }
      }
    } else {
      offer_address_host_order =
          ntohl(netconfig.fixed_hosts[request_hwaddr].s_addr);
    }
  }
  in_addr_t offer_address_netorder = htonl(offer_address_host_order);
  LOG_DEBUG(string_format("Offering address %s",
                          inet_ntoa({.s_addr = offer_address_netorder})));

  reply.assigned_ip = offer_address_host_order;
  reply.options[OptionTag::DHCP_MESSAGE_TYPE] = {DHCP_TYPE_OFFER};
  reply.options[OptionTag::SERVER_IDENTIFIER] =
      to_byte_vector(datagram.recv_addr);

  set_requested_options(datagram, reply);
  if (!reply.options.contains(OptionTag::LEASE_TIME)) {
    reply.options[OptionTag::LEASE_TIME] =
        to_byte_vector(netconfig.lease_time_seconds);
  }

  const uint64_t current_time_seconds = get_current_time();
  active_leases[datagram.hw_addr] =
      std::make_pair(offer_address_host_order, current_time_seconds + 10);

  struct sockaddr_in destination =
      get_reply_destination(datagram, offer_address_netorder);
  socket.enqueue_datagram(destination, reply);
}

void Daemon::handle_request(const DhcpDatagram &datagram) {
  in_addr_t requested_address_hostorder;
  if (datagram.options.contains(OptionTag::REQUESTED_IP_ADDRESS)) {
    requested_address_hostorder = to_number<in_addr_t>(
        datagram.options.at(OptionTag::REQUESTED_IP_ADDRESS));
  } else {
    requested_address_hostorder = datagram.client_ip;
  }
  in_addr_t requested_address_netorder = htonl(requested_address_hostorder);

  DhcpDatagram reply = create_skeleton_reply_datagram(datagram);
  if ((requested_address_netorder & netconfig.netmask.s_addr) !=
      netconfig.subnet_address.s_addr) {
    std::ostringstream os;
    os << "Requested address "
       << inet_ntoa(in_addr{.s_addr = requested_address_netorder})
       << " is not in the configured subnet!";
    LOG_WARN(os.str());
    reply.options[OptionTag::DHCP_MESSAGE_TYPE] = {DHCP_TYPE_NAK};
    struct sockaddr_in dest = get_reply_destination(datagram, INADDR_ANY);
    socket.enqueue_datagram(dest, reply);
    return;
  }
  update_leases();
  if (active_leases.contains(datagram.hw_addr)) {
    if (active_leases[datagram.hw_addr].first == requested_address_hostorder) {
      reply.options[OptionTag::DHCP_MESSAGE_TYPE] = {DHCP_TYPE_ACK};
      reply.assigned_ip = requested_address_hostorder;

      set_requested_options(datagram, reply);
      reply.options[OptionTag::SERVER_IDENTIFIER] =
          to_byte_vector(datagram.recv_addr);
      if (!reply.options.contains(OptionTag::LEASE_TIME)) {
        reply.options[OptionTag::LEASE_TIME] =
            to_byte_vector(netconfig.lease_time_seconds);
      }

      const uint64_t current_time_seconds = get_current_time();
      active_leases[datagram.hw_addr] =
          std::make_pair(requested_address_hostorder,
                         current_time_seconds + netconfig.lease_time_seconds);

      struct sockaddr_in destination =
          get_reply_destination(datagram, requested_address_netorder);

      LOG_DEBUG(string_format(
          "Assigned IP %s", inet_ntoa({.s_addr = requested_address_netorder})));

      socket.enqueue_datagram(destination, reply);
    } else {
      // if somebody else holds this lease, we tell the client to reset
      LOG_DEBUG("Requested address already in use");
      reply.options[OptionTag::DHCP_MESSAGE_TYPE] = {DHCP_TYPE_NAK};
      struct sockaddr_in dest = get_reply_destination(datagram, INADDR_ANY);
      socket.enqueue_datagram(dest, reply);
    }
  }
}

void Daemon::handle_release(const DhcpDatagram &datagram) {
  active_leases.erase(datagram.hw_addr);
}

void Daemon::handle_inform(const DhcpDatagram &datagram) {
  DhcpDatagram reply = create_skeleton_reply_datagram(datagram);
  set_requested_options(datagram, reply);
  reply.options.erase(OptionTag::LEASE_TIME);
  struct sockaddr_in destination =
      get_reply_destination(datagram, datagram.client_ip);
  socket.enqueue_datagram(destination, reply);
}

void Daemon::handle_decline(const DhcpDatagram &datagram) {
  in_addr_t declined_ip_hostorder = to_number<in_addr_t>(
      datagram.options.at(OptionTag::REQUESTED_IP_ADDRESS));
  std::array<uint8_t, 16> dummy_hwaddr;
  std::fill(dummy_hwaddr.begin(), dummy_hwaddr.end(), 0x0);
  active_leases[dummy_hwaddr] =
      std::make_pair(declined_ip_hostorder, UINT64_MAX);
}

// Determines the reply destination based on the request datagram.
// If the reply can be sent as unicast, this also injects the
// unicast_address <-> hwaddr mapping into the ARP cache.
struct sockaddr_in
Daemon::get_reply_destination(const DhcpDatagram &request_datagram,
                              const in_addr_t unicast_address) {
  struct sockaddr_in destination {
    .sin_family = AF_INET, .sin_port = htons(DHCP_CLIENT_PORT),
    .sin_addr = {.s_addr = unicast_address}, .sin_zero = {}
  };
  bool is_unicast = true;

  // is the broadcast flag set or the hw addr somehow not correct (e.g. all
  // zeroes)?
  if ((request_datagram.flags & 0x8000) != 0 ||
      request_datagram.hwaddr_len == 0 ||
      std::find_if(request_datagram.hw_addr.cbegin(),
                   request_datagram.hw_addr.cend(),
                   [](uint8_t byte) { return byte > 0; }) ==
          request_datagram.hw_addr.cend() ||
      unicast_address == INADDR_ANY) {
    is_unicast = false;
    struct ifreq ireq;
    std::memcpy(&ireq.ifr_name, request_datagram.recv_iface.c_str(),
                request_datagram.recv_iface.length() + 1);
    if (ioctl(socket, SIOCGIFBRDADDR, &ireq) != 0) {
      LOG_ERROR(string_format("Failed to get broadcast address! Falling back "
                              "to manual calculation. Error: %d",
                              errno));
      destination.sin_addr.s_addr =
          (request_datagram.recv_addr | ~netconfig.netmask.s_addr);
    } else {
      destination.sin_addr.s_addr =
          reinterpret_cast<struct sockaddr_in *>(&ireq.ifr_broadaddr)
              ->sin_addr.s_addr;
    }
  }
  if (is_unicast) {
    struct sockaddr arp_hwaddr {
      .sa_family = request_datagram.hwaddr_type, .sa_data = {}
    };
    std::copy(request_datagram.hw_addr.cbegin(),
              request_datagram.hw_addr.cbegin() + request_datagram.hwaddr_len,
              arp_hwaddr.sa_data);

    struct arpreq areq {
      .arp_pa = {}, .arp_ha = arp_hwaddr, .arp_flags = ATF_COM,
      .arp_netmask = {}, .arp_dev = {}
    };
    std::memcpy(&areq.arp_pa, &destination, sizeof(struct sockaddr_in));
    std::memcpy(&areq.arp_dev, request_datagram.recv_iface.c_str(),
                request_datagram.recv_iface.length() + 1);
    if (ioctl(socket, SIOCSARP, &areq) != 0)
      LOG_ERROR(
          string_format("Failed to inject into arp cache! Error: %d", errno));
  }
  return destination;
}

DhcpDatagram
Daemon::create_skeleton_reply_datagram(const DhcpDatagram &request_datagram) {
  DhcpDatagram skel{.opcode = 0x2,
                    .hwaddr_type = request_datagram.hwaddr_type,
                    .hwaddr_len = request_datagram.hwaddr_len,
                    .transaction_id = request_datagram.transaction_id,
                    .secs_passed = 0x0,
                    .flags = request_datagram.flags,
                    .client_ip = INADDR_ANY,
                    .assigned_ip = INADDR_ANY,
                    .server_ip = INADDR_ANY,
                    .relay_agent_ip = INADDR_ANY,
                    .recv_addr = 0x0,
                    .recv_iface = "",
                    .hw_addr = {},
                    .options =
                        std::unordered_map<OptionTag, std::vector<uint8_t>>()};
  std::copy(request_datagram.hw_addr.begin(), request_datagram.hw_addr.end(),
            skel.hw_addr.begin());
  return skel;
}

void Daemon::set_requested_options(const DhcpDatagram &request,
                                   DhcpDatagram &reply) {
  if (!request.options.contains(OptionTag::PARAMETER_REQUEST_LIST)) {
    return;
  }
  for (uint8_t option : request.options.at(OptionTag::PARAMETER_REQUEST_LIST)) {
    if (!netconfig.defined_options.contains(static_cast<OptionTag>(option)) ||
        reply.options.contains(static_cast<OptionTag>(option))) {
      LOG_TRACE(string_format("No configured value for option %x", option));
      continue;
    }
    reply.options[static_cast<OptionTag>(option)] =
        netconfig.defined_options.at(static_cast<OptionTag>(option));
    std::ostringstream os;
    os << string_format("Set value for option %x to ", option);
    for (auto &elem : reply.options[static_cast<OptionTag>(option)]) {
      os << string_format("%x", elem);
    }
    LOG_DEBUG(os.str());
  }
}

void Daemon::update_leases() {
  LOG_TRACE("Updating leases");
  const uint64_t current_time_seconds = get_current_time();
  for (auto map_iter = active_leases.cbegin();
       map_iter != active_leases.cend();) {
    if (map_iter->second.second <= current_time_seconds) {
      map_iter = active_leases.erase(map_iter);
    } else {
      ++map_iter;
    }
  }
}

void Daemon::load_leases() {
  std::ifstream lease_file(lease_file_path);
  if (!lease_file.is_open()) {
    LOG_WARN("The lease file does not exist! Creating a new one...");
    lease_file = std::ifstream(lease_file_path, std::fstream::trunc);
  }

  std::string current_line;
  const uint64_t current_time_seconds = get_current_time();
  LOG_DEBUG("Reading leases from file...");

  while (std::getline(lease_file, current_line)) {
    LOG_TRACE(current_line);
    size_t first_delim_pos = current_line.find_first_of(LEASE_FILE_DELIMITER);
    size_t last_delim_pos = current_line.find_last_of(LEASE_FILE_DELIMITER);

    if (first_delim_pos == std::string::npos ||
        last_delim_pos == std::string::npos) {
      LOG_WARN(string_format("Invalid entry in lease file: %s", current_line));
      continue;
    }

    std::string hwaddr_string = current_line.substr(0, first_delim_pos);
    std::string ipaddr_string = current_line.substr(
        first_delim_pos + 1, last_delim_pos - first_delim_pos - 1);
    uint64_t timeout_timestamp = std::strtoul(
        current_line
            .substr(last_delim_pos + 1, current_line.size() - last_delim_pos)
            .c_str(),
        nullptr, 10);

    std::ostringstream os;
    os << "IP: " << ipaddr_string << " | Ether: " << hwaddr_string
       << " | Timestamp: " << timeout_timestamp;
    LOG_DEBUG(os.str());
    if (timeout_timestamp < current_time_seconds) {
      continue;
    }

    std::array<uint8_t, 16> hwaddr;
    size_t index = 0;
    size_t pos = 0;
    while ((pos = hwaddr_string.find_first_of(':')) != std::string::npos) {
      hwaddr.at(index) =
          std::strtol(hwaddr_string.substr(0, pos).c_str(), nullptr, 16);
      hwaddr_string.erase(0, pos + 1); // account for delimiter length
      index++;
    }

    struct in_addr ip_addr {};
    inet_aton(ipaddr_string.c_str(), &ip_addr);
    active_leases[hwaddr] =
        std::make_pair(ntohl(ip_addr.s_addr), timeout_timestamp);
  }
  lease_file.close();
}

void Daemon::write_leases() {
  std::ofstream lease_file(lease_file_path);

  update_leases();
  struct in_addr ip_addr;
  LOG_DEBUG("Writing leases to file");
  for (auto const &[hwaddr, value_pair] : active_leases) {
    for (const uint8_t &octet : hwaddr) {
      lease_file << string_format("%x", octet) << ":";
    }
    lease_file << LEASE_FILE_DELIMITER;
    ip_addr.s_addr = htonl(value_pair.first);
    lease_file << inet_ntoa(ip_addr) << LEASE_FILE_DELIMITER
               << value_pair.second << "\n";
  }
  lease_file.flush();
}

uint64_t Daemon::get_current_time() {
  return static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
}
} // namespace tinydhcpd
