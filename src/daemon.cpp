#include "daemon.hpp"

#include "src/datagram.hpp"
#include "utils.hpp"
#include <arpa/inet.h>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <netinet/in.h>
#include <string>
#include <utility>

namespace tinydhcpd {
volatile std::sig_atomic_t last_signal;

constexpr uint8_t DHCP_TYPE_DISCOVER = 1;
constexpr uint8_t DHCP_TYPE_REQUEST = 3;
constexpr uint8_t DHCP_TYPE_RELEASE = 7;
constexpr uint8_t DHCP_TYPE_INFORM = 8;

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
}

void Daemon::handle_request(const DhcpDatagram &datagram) {
  DhcpDatagram reply = create_skeleton_reply_datagram(datagram);
  in_addr_t requested_address = to_number<in_addr_t>(datagram.options.at(OptionTag::REQUESTED_IP_ADDRESS));
  //TODO finish
}

DhcpDatagram
Daemon::create_skeleton_reply_datagram(const DhcpDatagram &request_datagram) {
  DhcpDatagram skel{.opcode = 0x2,
                    .hwaddr_type = request_datagram.hwaddr_type,
                    .hwaddr_len = request_datagram.hwaddr_len,
                    .transaction_id = request_datagram.transaction_id,
                    .server_ip = request_datagram.server_ip};
  std::copy(std::cbegin(request_datagram.hw_addr),
            std::cend(request_datagram.hw_addr), skel.hw_addr);
  return skel;
}

void Daemon::load_leases() {
  std::string current_line;
  const uint64_t current_time_seconds =
      std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count();

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
} // namespace tinydhcpd
