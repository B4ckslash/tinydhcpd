#include "socket.hpp"

#include <algorithm>
#include <arpa/inet.h>
#include <bits/types/struct_iovec.h>
#include <byteswap.h>
#include <cstdint>
#include <iterator>
#include <net/if.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "daemon.hpp"
#include "src/utils.hpp"

#define DGRAM_SIZE 576

//TODO refactor epoll into separate RAII class

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
  int enable = 1;
  if (setsockopt(socket_fd, IPPROTO_IP, IP_PKTINFO, &enable, sizeof(enable)) <
      0) {
    die("Failed to set socket option IP_PKTINFO: ");
  }
}

void Socket::bind_to_iface(const std::string iface_name) {
  struct ifreq ireq = {};
  std::snprintf(ireq.ifr_name, sizeof(ireq.ifr_name), "%s", iface_name.c_str());
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
  uint8_t control_msg_buffer[256];
  struct iovec data_buffer {
    .iov_base = raw_data_buffer, .iov_len = DGRAM_SIZE
  };
  struct msghdr message_header {
    .msg_name = nullptr, .msg_iov = &data_buffer, .msg_iovlen = 1,
    .msg_control = &control_msg_buffer, .msg_controllen = 256
  };
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
        if (recvmsg(socket_fd, &message_header, MSG_WAITALL) < 0) {
          continue;
        }
        try {
          DhcpDatagram datagram =
              DhcpDatagram::from_buffer(raw_data_buffer, DGRAM_SIZE);
          if (datagram.server_ip == static_cast<uint32_t>(0x0)) {
            datagram.server_ip = extract_destination_ip(message_header);
          }
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

uint32_t Socket::extract_destination_ip(struct msghdr &message_header) {
  for (struct cmsghdr *control_message = CMSG_FIRSTHDR(&message_header);
       control_message != nullptr;
       control_message = CMSG_NXTHDR(&message_header, control_message)) {
    if (control_message->cmsg_level != IPPROTO_IP ||
        control_message->cmsg_type != IP_PKTINFO) {
      continue;
    }
    struct in_pktinfo *packet_info =
        reinterpret_cast<struct in_pktinfo *>(CMSG_DATA(control_message));
    return bswap_32(packet_info->ipi_addr.s_addr);
  }
  return 0;
}

void Socket::enqueue_datagram(struct sockaddr &destination,
                              DhcpDatagram &datagram) {
  send_queue.push(std::make_pair(destination, datagram));
}
} // namespace tinydhcpd