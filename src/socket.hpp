#pragma once

#include <netinet/in.h>
#include <queue>
#include <string>

#include "socket_observer.hpp"

#define PORT 67

namespace tinydhcpd {
class Socket {
private:
  int socket_fd;
  SocketObserver &observer;
  const struct sockaddr_in listen_address;
  std::queue<std::pair<struct sockaddr, DhcpDatagram>> send_queue;
  void die(std::string error_msg);
  uint32_t extract_destination_ip(struct msghdr &message_header);

public:
  Socket(const struct in_addr &address, const std::string &iface_name,
         SocketObserver &observer);
  ~Socket() noexcept;
  operator int();
  void enqueue_datagram(struct sockaddr &destination, DhcpDatagram &datagram);
  void handle_epollin();
  void handle_epollout();
};

} // namespace tinydhcpd