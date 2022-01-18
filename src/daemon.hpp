#pragma once

#include <netinet/in.h>

#include "epoll.hpp"
#include "socket.hpp"
#include "socket_observer.hpp"
#include "subnet_config.hpp"

namespace tinydhcpd {
class Daemon : SocketObserver {
private:
  Socket socket;
  Epoll<Socket> epoll_socket;
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