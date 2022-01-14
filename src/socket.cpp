#include "socket.hpp"

#include <arpa/inet.h>
#include <net/if.h>
#include <string.h>
#include <unistd.h>

#include "daemon.hpp"

#define DGRAM_SIZE 576

namespace tinydhcpd {
Socket::Socket(const struct in_addr &address, const std::string &iface_name,
               SocketObserver &observer)
    : observer(observer), listen_address{.sin_family = AF_INET,
                                         .sin_port = htons(PORT),
                                         .sin_addr = address,
                                         .sin_zero = {}},
      send_queue() {
  create_socket();
  if (!iface_name.empty()) {
    bind_to_iface(iface_name);
  }
  bind_to_address(listen_address);
  setup_epoll();
}

Socket::~Socket() noexcept {
  close(epoll_fd);
  close(socket_fd);
}

void Socket::die(std::string error_msg) {
  throw std::runtime_error(error_msg.append(strerror(errno)));
}

void Socket::create_socket() {
  socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (socket_fd == -1) {
    die("Failed to create socket: ");
  }
}

void Socket::bind_to_iface(const std::string iface_name) {
  struct ifreq ireq = {};
  snprintf(ireq.ifr_name, sizeof(ireq.ifr_name), "%s", iface_name.c_str());
  if (setsockopt(socket_fd, SOL_SOCKET, SO_BINDTODEVICE, (const void *)&ireq,
                 sizeof(ireq)) < 0) {
    std::string msg("Failed to bind to interface ");
    msg.append(iface_name);
    msg.append(": ");
    die(msg);
  }
}

void Socket::bind_to_address(const struct sockaddr_in &address) {
  if (bind(socket_fd, (const sockaddr *)&address, sizeof(address)) == -1) {
    die("Failed to bind socket: ");
  }
}

void Socket::setup_epoll() {
  epoll_fd = epoll_create1(0);
  if (epoll_fd == -1) {
    die("Failed to create epoll file descriptor: ");
  }
  ev.events = EPOLLIN | EPOLLOUT;
  ev.data.fd = socket_fd;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &ev) == -1) {
    die("Failed at epoll_ctl: ");
  }
}

void Socket::recv_loop() {
  int number_ready_fds;
  uint8_t raw_data_buffer[DGRAM_SIZE];
  std::fill(raw_data_buffer, raw_data_buffer + DGRAM_SIZE, (uint8_t)0);
  while (true) {
    if (tinydhcpd::last_signal == SIGINT || tinydhcpd::last_signal == SIGTERM) {
      return;
    }

    number_ready_fds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    if (number_ready_fds == -1) {
      if (errno == EINTR) {
        continue;
      }

      die("epoll_wait failed: ");
    }
    for (int n = 0; n < number_ready_fds; n++) {
      if ((events[n].events & EPOLLIN) > 0) {
        if (recv(socket_fd, raw_data_buffer, DGRAM_SIZE, MSG_WAITALL) < 0) {
          continue;
        }
        try {
          DhcpDatagram datagram(raw_data_buffer, DGRAM_SIZE);
          observer.handle_recv(datagram);
        } catch (std::invalid_argument &ex) {
          std::cerr << ex.what() << std::endl;
        }
        std::fill(raw_data_buffer, raw_data_buffer + DGRAM_SIZE, (uint8_t)0);
      } else if ((events[n].events & EPOLLOUT) > 0 && !send_queue.empty()) {
        struct sockaddr dst = send_queue.front().first;
        std::vector byte_datagram = send_queue.front().second.to_byte_vector();
        size_t datagram_size = byte_datagram.size();
        if (sendto(socket_fd, byte_datagram.data(), datagram_size, MSG_DONTWAIT,
                   &dst, sizeof(dst)) < 0) {
          continue;
        }
        send_queue.pop();
      }
    }
  }
}

void Socket::enqueue_datagram(struct sockaddr &destination,
                              DhcpDatagram &datagram) {
  send_queue.push(std::make_pair(destination, datagram));
}
} // namespace tinydhcpd