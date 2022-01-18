#pragma once

#include <map>
#include <netinet/in.h>
#include <vector>

namespace tinydhcpd {
enum struct OptionTag : uint8_t {
  PAD = 0,
  SUBNET_MASK = 1,
  TIME_OFFSET = 2,
  ROUTERS = 3,
  TIME_SERVER = 4,
  DNS_SERVER = 5,
  LOG_SERVER = 6,
  HOSTNAME = 12,
  DOMAIN_NAME = 15,
  IFACE_MTU = 26,
  BROADCAST_ADDR = 28,
  STATIC_ROUTES = 33,
  REQUESTED_IP_ADDRESS = 50,
  DHCP_MESSAGE_TYPE = 53,
  OPTIONS_END = 255
};

struct DhcpDatagram {
  uint8_t opcode;
  uint8_t hwaddr_type;
  uint8_t hwaddr_len;

  uint32_t transaction_id;
  uint16_t secs_passed;
  uint16_t flags;

  uint32_t client_ip;
  uint32_t assigned_ip;
  uint32_t server_ip;
  uint32_t relay_agent_ip;

  uint8_t hw_addr[16];

  std::map<OptionTag, std::vector<uint8_t>> options;

  static DhcpDatagram from_buffer(uint8_t *buffer, int buflen);

  std::vector<uint8_t> to_byte_vector();
  static std::map<OptionTag, std::vector<uint8_t>>
  parse_options(const uint8_t *options_buffer, size_t buffer_size);
};

uint16_t convert_network_byte_array_to_uint16(uint8_t *array);
uint32_t convert_network_byte_array_to_uint32(uint8_t *array);
} // namespace tinydhcpd