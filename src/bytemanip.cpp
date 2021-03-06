#include "bytemanip.hpp"

#include <arpa/inet.h>

namespace tinydhcpd {
template <typename N, size_t size>
std::array<uint8_t, size> __to_big_endian_byte_array(N number) {
  N network_byte_order_number;
  if (size == 4) {
    network_byte_order_number = htonl(number);
  } else {
    network_byte_order_number = htons(number);
  }

  return to_byte_array<>(network_byte_order_number);
}

std::array<uint8_t, 4> to_network_byte_array(uint32_t number) {
  return __to_big_endian_byte_array<uint32_t, 4>(number);
}
std::array<uint8_t, 2> to_network_byte_array(uint16_t number) {
  return __to_big_endian_byte_array<uint16_t, 2>(number);
}

uint16_t convert_network_byte_array_to_uint16(uint8_t *array) {
  return ntohs(to_number<uint16_t>(array));
}

uint32_t convert_network_byte_array_to_uint32(uint8_t *array) {
  return ntohl(to_number<uint32_t>(array));
}
} // namespace tinydhcpd
