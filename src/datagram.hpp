#pragma once

#include <map>
#include <netinet/in.h>
#include <string>
#include <vector>

#include "utils.hpp"

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
  SERVER_IDENTIFIER = 54,
  DHCP_RENEW_TIME = 58,
  DHCP_REBINDING_TIME = 59,
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

  in_addr_t recv_addr;
  std::string recv_iface;

  std::array<uint8_t, 16> hw_addr;

  std::map<OptionTag, std::vector<uint8_t>> options;

  static DhcpDatagram from_buffer(uint8_t *buffer, size_t buflen);

  std::vector<uint8_t> to_byte_vector();
  static std::map<OptionTag, std::vector<uint8_t>>
  parse_options(const uint8_t *options_buffer, size_t buffer_size);
};

uint16_t convert_network_byte_array_to_uint16(uint8_t *array);
uint32_t convert_network_byte_array_to_uint32(uint8_t *array);

template <typename N>
void convert_number_to_network_byte_array_and_push(std::vector<uint8_t> &vec,
                                                   N number) {
  auto network_order_array = to_network_byte_array(number);
  for (uint8_t &byte : network_order_array) {
    vec.push_back(byte);
  }
}
template <typename N>
void convert_number_to_byte_array_and_push(std::vector<uint8_t> &vec,
                                           N number) {
  auto byte_array = to_byte_array(number);
  for (uint8_t &byte : byte_array) {
    vec.push_back(byte);
  }
}
} // namespace tinydhcpd
