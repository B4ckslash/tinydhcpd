#pragma once

#include <netinet/in.h>
#include <unordered_map>

#include "bytemanip.hpp"

namespace tinydhcpd {
enum struct OptionTag : uint8_t {
  PAD = 0,
  SUBNET_MASK = 1,
  TIME_OFFSET = 2,
  ROUTERS = 3,
  TIME_SERVER = 4,
  NAME_SERVER = 5,
  DNS_SERVER = 6,
  LOG_SERVER = 7,
  HOSTNAME = 12,
  DOMAIN_NAME = 15,
  IFACE_MTU = 26,
  BROADCAST_ADDR = 28,
  STATIC_ROUTES = 33,
  REQUESTED_IP_ADDRESS = 50,
  LEASE_TIME = 51,
  DHCP_MESSAGE_TYPE = 53,
  SERVER_IDENTIFIER = 54,
  PARAMETER_REQUEST_LIST = 55,
  DHCP_RENEW_TIME = 58,
  DHCP_REBINDING_TIME = 59,
  OPTIONS_END = 255
};

struct DhcpDatagram {
  uint8_t _opcode;
  uint8_t _hwaddr_type;
  uint8_t _hwaddr_len;

  uint32_t _transaction_id;
  uint16_t _secs_passed;
  uint16_t _flags;

  uint32_t _client_ip;
  uint32_t _assigned_ip;
  uint32_t _server_ip;
  uint32_t _relay_agent_ip;

  in_addr_t _recv_addr;
  std::string _recv_iface;

  std::array<uint8_t, 16> _hw_addr;

  std::unordered_map<OptionTag, std::vector<uint8_t>> _options;

  static DhcpDatagram from_buffer(uint8_t *buffer, size_t buflen);

  std::vector<uint8_t> to_byte_vector();
  static std::unordered_map<OptionTag, std::vector<uint8_t>>
  parse_options(const uint8_t *options_buffer, size_t buffer_size);
};

} // namespace tinydhcpd
