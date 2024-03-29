#pragma once

#include <queue>

#include "socket_observer.hpp"

#define PORT 67

namespace tinydhcpd {
class Socket {
private:
  int _socket_fd;
  SocketObserver &_observer;
  const struct sockaddr_in _listen_address;
  in_addr_t _server_ip;
  std::queue<std::pair<struct sockaddr_in, DhcpDatagram>> _send_queue;
  [[noreturn]] void die(std::string error_msg);
  std::pair<in_addr_t, std::string>
  extract_interface_info(struct msghdr &message_header);

public:
  Socket(const struct in_addr &address, const std::string &iface_name,
         SocketObserver &observer);
  ~Socket() noexcept;
  Socket(Socket &&other) noexcept = default;
  // forbid copy construction, only one socket
  // instance should be wrapping the fd
  Socket(Socket &other) = delete;

  operator int();

  void enqueue_datagram(struct sockaddr_in &destination,
                        DhcpDatagram &datagram);
  bool has_waiting_messages();
  bool handle_epollin();
  bool handle_epollout();
};

} // namespace tinydhcpd
