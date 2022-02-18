#pragma once

#include <array>
#include <iostream>
#include <vector>

namespace tinydhcpd {

std::array<uint8_t, 4> to_network_byte_array(uint32_t number);
std::array<uint8_t, 2> to_network_byte_array(uint16_t number);
uint16_t convert_network_byte_array_to_uint16(uint8_t *array);
uint32_t convert_network_byte_array_to_uint32(uint8_t *array);

template <typename N>
std::array<uint8_t, sizeof(N) / sizeof(uint8_t)> to_byte_array(N number) {
  const size_t size = sizeof(N) / sizeof(uint8_t);
  std::array<uint8_t, size> arr;
  for (size_t i = 0; i < size; i++) {
    uint8_t shift =
        8 * (size - i - 1); // shift value must decrease with increasing array
                            // index to preserve endianness
    arr[i] = (number & (((N)0xff) << shift)) >> shift;
  }
  return arr;
}

template <typename N> N to_number(std::vector<uint8_t> bytes) {
  constexpr size_t max_size = sizeof(N) / sizeof(uint8_t);
  if (bytes.size() != max_size) {
    throw std::invalid_argument("The given vector is not the right size!");
  }
  N value = 0;
  for (size_t i = 0; i < max_size; i++) {
    uint8_t shift = 8 * (max_size - i - 1);
    value |= (bytes[i] << shift);
  }
  return value;
}

template <typename N> N to_number(uint8_t *bytes) {
  constexpr size_t size = sizeof(N) / sizeof(uint8_t);
  N value = 0;
  for (size_t i = 0; i < size; i++) {
    uint8_t shift = 8 * (size - i - 1);
    value |= (bytes[i] << shift);
  }
  return value;
}

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
