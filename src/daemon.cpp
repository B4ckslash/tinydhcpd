#include "daemon.hpp"

#include <arpa/inet.h>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <linux/if_packet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <utility>

#include "datagram.hpp"
#include "utils.hpp"

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

Daemon::Daemon(const struct in_addr &address, const std::string &iface_name,
               SubnetConfiguration &netconfig,
               const std::string &lease_file_path) try
    : socket(address, iface_name, *this),
      epoll_socket(socket, (EPOLLIN | EPOLLOUT)), netconfig(netconfig),
      lease_file(lease_file_path, std::fstream::in | std::fstream::out),
      active_leases() {
  load_leases();
  epoll_socket.poll_loop();
} catch (std::runtime_error &ex) {
  printf("%s\n", ex.what());
  exit(EXIT_FAILURE);
}

void Daemon::handle_recv(DhcpDatagram &datagram) {
  std::cout << string_format("XID: %#010x", datagram.transaction_id)
            << std::endl;
  if (datagram.opcode != 0x1) {
    return;
  }
  std::for_each(datagram.options.begin(), datagram.options.end(),
                [](const auto &option) {
                  std::cout << string_format("Tag %u | Length %u | Value(s) ",
                                             static_cast<uint8_t>(option.first),
                                             option.second.size());
                  for (uint8_t val_byte : option.second) {
                    std::cout << string_format("%#04x ", val_byte);
                  }
                  std::cout << std::endl;
                });

  switch (datagram.options[OptionTag::DHCP_MESSAGE_TYPE].at(0)) {
  case DHCP_TYPE_DISCOVER:
    handle_discovery(datagram);
    break;
  case DHCP_TYPE_REQUEST:
    std::cout << "DHCPREQUEST" << std::endl;
    handle_request(datagram);
    break;
  case DHCP_TYPE_RELEASE:
    break;
  case DHCP_TYPE_INFORM:
    break;
  default:
    break;
  }
}

void Daemon::handle_discovery(const DhcpDatagram &datagram) {
  DhcpDatagram reply = create_skeleton_reply_datagram(datagram);
  std::cout << "DHCPDISCOVER" << std::endl;
  in_addr_t offer_address_host_order = ntohl(netconfig.range_start.s_addr);
  in_addr_t range_end_host_order = ntohl(netconfig.range_end.s_addr);
  update_leases();
  for (auto &entry : active_leases) {
    if (offer_address_host_order == entry.first) {
      offer_address_host_order++;
    }
    if (offer_address_host_order >= range_end_host_order) {
      return;
    } else {
      break;
    }
  }
  in_addr_t offer_address_netorder = htonl(offer_address_host_order);

  reply.assigned_ip = offer_address_host_order;
  reply.options[OptionTag::DHCP_MESSAGE_TYPE] = {DHCP_TYPE_OFFER};

  auto renew_time_network_order_array =
      to_byte_array(netconfig.lease_time_seconds / 2);
  auto rebind_time_network_order_array =
      to_byte_array(netconfig.lease_time_seconds);
  reply.options[OptionTag::DHCP_RENEW_TIME] =
      std::vector<uint8_t>(renew_time_network_order_array.begin(),
                           renew_time_network_order_array.end());
  reply.options[OptionTag::DHCP_REBINDING_TIME] =
      std::vector<uint8_t>(rebind_time_network_order_array.begin(),
                           rebind_time_network_order_array.end());

  const uint64_t current_time_seconds = get_current_time();
  active_leases[offer_address_netorder] =
      std::make_pair(datagram.hw_addr, current_time_seconds + 10);

  struct sockaddr_in destination {
    .sin_family = AF_INET, .sin_port = htons(DHCP_CLIENT_PORT), .sin_addr = {},
    .sin_zero = {}
  };
  if ((datagram.flags & 0x8000) != 0) {
    struct ifreq ireq;
    std::memcpy(&ireq.ifr_name, datagram.recv_iface.c_str(),
                datagram.recv_iface.length() + 1);
    if (ioctl(socket, SIOCGIFBRDADDR, &ireq) != 0) {
      std::cerr << "Failed to get broadcast address! Falling back to manual "
                   "calculation. Error: "
                << errno << std::endl;
      destination.sin_addr.s_addr =
          ((datagram.recv_addr & netconfig.netmask.s_addr) |
           ~netconfig.netmask.s_addr);
    } else {
      destination.sin_addr.s_addr =
          reinterpret_cast<struct sockaddr_in *>(&ireq.ifr_broadaddr)
              ->sin_addr.s_addr;
    }
  } else {
    destination.sin_addr.s_addr = offer_address_netorder;
    inject_into_arp(destination, datagram);
  }
  socket.enqueue_datagram(destination, reply);
}

void Daemon::handle_request(const DhcpDatagram &datagram) {
  DhcpDatagram reply = create_skeleton_reply_datagram(datagram);
  in_addr_t requested_address_netorder = htonl(to_number<in_addr_t>(
      datagram.options.at(OptionTag::REQUESTED_IP_ADDRESS)));
  if ((requested_address_netorder & netconfig.netmask.s_addr) !=
      netconfig.subnet_address.s_addr) {
    std::cerr << "Requested address "
              << inet_ntoa(in_addr{.s_addr = requested_address_netorder})
              << " is not in range!" << std::endl;
    return;
  }
  update_leases();
  if (!active_leases.contains(requested_address_netorder) ||
      active_leases[requested_address_netorder].first == datagram.hw_addr) {
    reply.options[OptionTag::DHCP_MESSAGE_TYPE] = {DHCP_TYPE_ACK};
    reply.assigned_ip = ntohl(requested_address_netorder);

    auto renew_time_network_order_array =
        to_byte_array(netconfig.lease_time_seconds / 2);
    auto rebind_time_network_order_array =
        to_byte_array(netconfig.lease_time_seconds);
    reply.options[OptionTag::DHCP_RENEW_TIME] =
        std::vector<uint8_t>(renew_time_network_order_array.begin(),
                             renew_time_network_order_array.end());
    reply.options[OptionTag::DHCP_REBINDING_TIME] =
        std::vector<uint8_t>(rebind_time_network_order_array.begin(),
                             rebind_time_network_order_array.end());

    const uint64_t current_time_seconds = get_current_time();
    active_leases[requested_address_netorder] = std::make_pair(
        datagram.hw_addr, current_time_seconds + netconfig.lease_time_seconds);

    struct sockaddr_in destination {
      .sin_family = AF_INET, .sin_port = htons(DHCP_CLIENT_PORT),
      .sin_addr = {.s_addr = requested_address_netorder},
    };

    inject_into_arp(destination, datagram);

    socket.enqueue_datagram(destination, reply);
  }
}

void Daemon::inject_into_arp(const struct sockaddr_in &destination,
                             const DhcpDatagram &request_datagram) {
  struct sockaddr arp_hwaddr {
    .sa_family = request_datagram.hwaddr_type, .sa_data = {}
  };
  std::copy(request_datagram.hw_addr.cbegin(),
            request_datagram.hw_addr.cbegin() + request_datagram.hwaddr_len,
            arp_hwaddr.sa_data);

  struct arpreq areq {
    .arp_pa = {}, .arp_ha = arp_hwaddr, .arp_flags = ATF_COM, .arp_netmask = {},
    .arp_dev = {}
  };
  std::memcpy(&areq.arp_pa, &destination, sizeof(struct sockaddr_in));
  std::memcpy(&areq.arp_dev, request_datagram.recv_iface.c_str(),
              request_datagram.recv_iface.length() + 1);
  if (ioctl(socket, SIOCSARP, &areq) != 0)
    std::cerr << "Failed to inject into arp cache!" << errno << std::endl;
}

DhcpDatagram
Daemon::create_skeleton_reply_datagram(const DhcpDatagram &request_datagram) {
  DhcpDatagram skel{.opcode = 0x2,
                    .hwaddr_type = request_datagram.hwaddr_type,
                    .hwaddr_len = request_datagram.hwaddr_len,
                    .transaction_id = request_datagram.transaction_id,
                    .flags = request_datagram.flags,
                    .server_ip = request_datagram.recv_addr};
  std::copy(request_datagram.hw_addr.begin(), request_datagram.hw_addr.end(),
            skel.hw_addr.begin());
  return skel;
}

void Daemon::update_leases() {
  const uint64_t current_time_seconds = get_current_time();
  for (auto map_iter = active_leases.cbegin();
       map_iter != active_leases.cend();) {
    if (map_iter->second.second <= current_time_seconds) {
      active_leases.erase(map_iter++);
    } else {
      ++map_iter;
    }
  }
}

void Daemon::load_leases() {
  std::string current_line;
  const uint64_t current_time_seconds = get_current_time();

  while (std::getline(lease_file, current_line)) {
    size_t first_delim_pos = current_line.find_first_of(LEASE_FILE_DELIMITER);
    size_t last_delim_pos = current_line.find_last_of(LEASE_FILE_DELIMITER);

    if (first_delim_pos == std::string::npos ||
        last_delim_pos == std::string::npos) {
      std::cerr << "Invalid entry in lease file: " << current_line << std::endl;
    }

    std::string hwaddr_string = current_line.substr(0, first_delim_pos);
    std::string ipaddr_string =
        current_line.substr(first_delim_pos + 1, last_delim_pos);
    uint64_t timeout_timestamp = std::strtoull(
        current_line
            .substr(last_delim_pos + 1, current_line.size() - last_delim_pos)
            .c_str(),
        nullptr, 16);

    if (timeout_timestamp < current_time_seconds) {
      continue;
    }

    std::array<uint8_t, 16> hwaddr;
    size_t index = 0;
    while (size_t pos = hwaddr_string.find_first_of(":") != std::string::npos) {
      hwaddr[index] =
          std::strtol(hwaddr_string.substr(0, pos).c_str(), nullptr, 16);
    }

    struct in_addr ip_addr {};
    inet_aton(ipaddr_string.c_str(), &ip_addr);
    active_leases[ip_addr.s_addr] = std::make_pair(hwaddr, timeout_timestamp);
  }
}

uint64_t Daemon::get_current_time() {
  return static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
}
} // namespace tinydhcpd
