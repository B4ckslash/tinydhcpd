#include "socket.hpp"

#include <byteswap.h>
#include <net/if.h>
#include <string.h>

#include "utils.hpp"

#define DGRAM_SIZE 576

namespace tinydhcpd {
Socket::Socket(const struct in_addr &address, const std::string &iface_name,
               SocketObserver &observer)
    : observer(observer), listen_address{.sin_family = AF_INET,
                                         .sin_port = htons(PORT),
                                         .sin_addr = address,
                                         .sin_zero = {}},
      send_queue() {
  socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (socket_fd == -1) {
    die("Failed to create socket: ");
  }
  int enable = 1;
  if (setsockopt(socket_fd, IPPROTO_IP, IP_PKTINFO, &enable, sizeof(enable)) <
      0) {
    die("Failed to set socket option IP_PKTINFO: ");
  }

  if (!iface_name.empty()) {
    struct ifreq ireq {};
    std::snprintf(ireq.ifr_name, sizeof(ireq.ifr_name), "%s",
                  iface_name.c_str());
    if (setsockopt(socket_fd, SOL_SOCKET, SO_BINDTODEVICE, (const void *)&ireq,
                   sizeof(ireq)) < 0) {
      std::string msg("Failed to bind to interface ");
      msg.append(iface_name);
      msg.append(": ");
      die(msg);
    }
  }

  if (bind(socket_fd, (const sockaddr *)&listen_address,
           sizeof(listen_address)) == -1) {
    die("Failed to bind socket: ");
  }
}

Socket::~Socket() noexcept { close(socket_fd); }

Socket::operator int() { return socket_fd; }

void Socket::die(std::string error_msg) {
  throw std::runtime_error(error_msg.append(strerror(errno)));
}

void Socket::handle_epollin() {
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

  if (recvmsg(socket_fd, &message_header, MSG_WAITALL) < 0) {
    return;
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
}

void Socket::handle_epollout() {
  if (send_queue.empty())
    return;

  struct sockaddr destination = send_queue.front().first;
  std::vector<uint8_t> data = send_queue.front().second.to_byte_vector();
  sendto(socket_fd, data.data(), data.size(), MSG_DONTWAIT, &destination,
         sizeof(destination));
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