#pragma once

#include <cstdint>
#include <fstream>
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
  std::fstream lease_file;
  std::map<std::array<uint8_t, 16>, std::pair<in_addr_t, uint64_t>>
      active_leases;
  void load_leases();
  void update_leases();
  uint64_t get_current_time();
  struct sockaddr_in get_reply_destination(const DhcpDatagram &request_datagram,
                                           const in_addr_t unicast_address);
  void set_requested_options(const DhcpDatagram &request, DhcpDatagram &reply);
  void handle_discovery(const DhcpDatagram &datagram);
  void handle_request(const DhcpDatagram &datagram);
  void handle_decline(const DhcpDatagram &datagram);
  void handle_release(const DhcpDatagram &datagram);
  void handle_inform(const DhcpDatagram &datagram);

public:
  Daemon(const struct in_addr &address, const std::string &iface_name,
         SubnetConfiguration &netconfig, const std::string &lease_file_path);
  virtual ~Daemon() {}
  virtual void handle_recv(DhcpDatagram &datagram) override;
  void main_loop();
  void write_leases();
};

} // namespace tinydhcpd
