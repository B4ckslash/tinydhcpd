#include "socket.hpp"

#include <arpa/inet.h>
#include <net/if.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "bytemanip.hpp"
#include "log/logger.hpp"
#include "string-format.hpp"

#define DGRAM_SIZE 576

namespace tinydhcpd {
Socket::Socket(const struct in_addr &address, const std::string &iface_name,
               SocketObserver &observer)
    : _observer(observer), _listen_address{.sin_family = AF_INET,
                                           .sin_port = htons(PORT),
                                           .sin_addr = address,
                                           .sin_zero = {}},
      _server_ip(address.s_addr), _send_queue() {
  _socket_fd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
  if (_socket_fd == -1) {
    die("Failed to create socket: ");
  }
  int enable = 1;
  if (setsockopt(_socket_fd, IPPROTO_IP, IP_PKTINFO, &enable, sizeof(enable)) <
      0) {
    die("Failed to set socket option IP_PKTINFO: ");
  }
  if (setsockopt(_socket_fd, SOL_SOCKET, SO_BROADCAST, &enable,
                 sizeof(enable)) < 0) {
    die("Failed to set socket option SO_BROADCAST: ");
  }

  if (!iface_name.empty()) {
    struct ifreq ireq {};
    ireq.ifr_addr.sa_family = AF_INET;
    std::copy(iface_name.begin(), iface_name.end(), ireq.ifr_name);
    if (setsockopt(_socket_fd, SOL_SOCKET, SO_BINDTODEVICE, (const void *)&ireq,
                   sizeof(ireq)) < 0) {
      std::string msg("Failed to bind to interface ");
      msg.append(iface_name);
      msg.append(": ");
      die(msg);
    }
    LOG_INFO(string_format("Binding to interface %s", iface_name.c_str()));
    if (_server_ip == INADDR_ANY) {
      if (ioctl(_socket_fd, SIOCGIFADDR, &ireq) < 0) {
        die("ioctl SIOCGIFADDR failed: ");
      }
      _server_ip = ((struct sockaddr_in *)&ireq.ifr_addr)->sin_addr.s_addr;
    }
  }

  if (bind(_socket_fd, (const sockaddr *)&_listen_address,
           sizeof(_listen_address)) == -1) {
    die("Failed to bind socket: ");
  }
  LOG_INFO(string_format("Listening on address %s",
                         inet_ntoa({.s_addr = _server_ip})));
  if (!iface_name.empty()) {
  }
}

Socket::~Socket() noexcept { close(_socket_fd); }

Socket::operator int() { return _socket_fd; }

void Socket::die(std::string error_msg) {
  LOG_FATAL(error_msg.append(strerror(errno)));
  throw std::runtime_error("");
}

bool Socket::handle_epollin() {
  uint8_t raw_data_buffer[DGRAM_SIZE];
  uint8_t control_msg_buffer[256];
  struct iovec data_buffer {
    .iov_base = raw_data_buffer, .iov_len = DGRAM_SIZE
  };
  struct msghdr message_header {
    .msg_name = nullptr, .msg_namelen = 0, .msg_iov = &data_buffer,
    .msg_iovlen = 1,
#ifdef __MUSL__
    // musl libc adheres more strictly to POSIX, which necessitates some padding
    // Functionally this is the same as default initialization, but I don't like
    // the compiler complaining if the field initializers are missing
        .__pad1 = 0,
#endif
    .msg_control = &control_msg_buffer, .msg_controllen = 256,
#ifdef __MUSL__
    .__pad2 = 0,
#endif
    .msg_flags = 0,
  };
  std::fill(raw_data_buffer, raw_data_buffer + DGRAM_SIZE, (uint8_t)0);

  if (recvmsg(_socket_fd, &message_header, MSG_WAITALL) < 0) {
    if (errno == EWOULDBLOCK) {
      return true;
    }
    return false;
  }
  try {
    DhcpDatagram datagram =
        DhcpDatagram::from_buffer(raw_data_buffer, DGRAM_SIZE);
    if (datagram._server_ip == static_cast<uint32_t>(0x0)) {
      datagram._server_ip = _server_ip;
    }
    auto iface_info = extract_interface_info(message_header);
    datagram._recv_addr = iface_info.first;
    datagram._recv_iface = iface_info.second;
    _observer.handle_recv(datagram);
  } catch (std::invalid_argument &ex) {
    LOG_ERROR(ex.what());
  }
  std::fill(raw_data_buffer, raw_data_buffer + DGRAM_SIZE, (uint8_t)0);
  return false;
}

bool Socket::handle_epollout() {
  if (_send_queue.empty()) {
    return false;
  }

  struct sockaddr_in destination = _send_queue.front().first;
  std::vector<uint8_t> data = _send_queue.front().second.to_byte_vector();
  if (sendto(_socket_fd, data.data(), data.size(), MSG_DONTWAIT,
             reinterpret_cast<struct sockaddr *>(&destination),
             sizeof(destination)) == -1) {
    if (errno == EWOULDBLOCK) {
      return true;
    } else {
      LOG_ERROR("Send failed!");
    }
  }
  _send_queue.pop();
  return false;
}

bool Socket::has_waiting_messages() { return _send_queue.size() > 0; }

std::pair<in_addr_t, std::string>
Socket::extract_interface_info(struct msghdr &message_header) {
  for (struct cmsghdr *control_message = CMSG_FIRSTHDR(&message_header);
       control_message != nullptr;
       control_message = CMSG_NXTHDR(&message_header, control_message)) {
    if (control_message->cmsg_level != IPPROTO_IP ||
        control_message->cmsg_type != IP_PKTINFO) {
      continue;
    }
    struct in_pktinfo *packet_info =
        reinterpret_cast<struct in_pktinfo *>(CMSG_DATA(control_message));
    const int if_index = packet_info->ipi_ifindex;
    struct ifreq ireq {};
    ireq.ifr_addr.sa_family = AF_INET;
    ireq.ifr_ifindex = if_index;
    if (ioctl(_socket_fd, SIOCGIFNAME, &ireq) != 0)
      die("Failed to get interface name: ");
    ioctl(_socket_fd, SIOCGIFADDR, &ireq);
    std::string iface_name(ireq.ifr_name);
    return std::make_pair(
        ntohl(reinterpret_cast<struct sockaddr_in *>(&ireq.ifr_addr)
                  ->sin_addr.s_addr),
        iface_name);
  }
  die("No control message with packet info!");
}

void Socket::enqueue_datagram(struct sockaddr_in &destination,
                              DhcpDatagram &datagram) {
  _send_queue.push(std::make_pair(destination, datagram));
}
} // namespace tinydhcpd
