#pragma once

#include "socket.hpp"

#include <netinet/in.h>

#include <csignal>
#include <iostream>

#include "socket_observer.hpp"
#include "src/datagram.hpp"
#include "subnet_config.hpp"

namespace tinydhcpd {
extern volatile std::sig_atomic_t last_signal;
class Daemon : SocketObserver {
private:
  Socket socket;
  SubnetConfiguration netconfig;
  DhcpDatagram
  create_skeleton_reply_datagram(const DhcpDatagram &request_datagram);
  void handle_discovery(const DhcpDatagram &datagram);

public:
  Daemon(const struct in_addr &address, const std::string &iface_name,
         SubnetConfiguration &netconfig);
  virtual ~Daemon() {}
  virtual void handle_recv(DhcpDatagram &datagram) override;
};

} // namespace tinydhcpd