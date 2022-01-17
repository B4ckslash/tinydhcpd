#include "daemon.hpp"

#include <algorithm>
#include <arpa/inet.h>
#include <cstdint>

#include "src/datagram.hpp"
#include "utils.hpp"

namespace tinydhcpd {
volatile std::sig_atomic_t last_signal;

constexpr uint8_t DHCP_TYPE_DISCOVER = 1;
constexpr uint8_t DHCP_TYPE_REQUEST = 3;
constexpr uint8_t DHCP_TYPE_RELEASE = 7;
constexpr uint8_t DHCP_TYPE_INFORM = 8;

Daemon::Daemon(const struct in_addr &address, const std::string &iface_name,
               SubnetConfiguration &netconfig) try
    : socket{address, iface_name, *this}, netconfig(netconfig) {
  socket.recv_loop();
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
} // namespace tinydhcpd
