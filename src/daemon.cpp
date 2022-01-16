#include "daemon.hpp"

#include <arpa/inet.h>

#include "src/datagram.hpp"
#include "utils.hpp"

namespace tinydhcpd {
volatile std::sig_atomic_t last_signal;

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
}
} // namespace tinydhcpd
