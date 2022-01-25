#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace tinydhcpd {
// from
// https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf
template <typename... Args>
std::string string_format(const std::string &format, Args... args) {
  static_assert(sizeof...(Args) > 0, "At least one argument is needed!");
  int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) +
               1; // Extra space for '\0'
  if (size_s <= 0) {
    throw std::runtime_error("Error during formatting.");
  }
  auto size = static_cast<size_t>(size_s);
  auto buf = std::make_unique<char[]>(size);
  std::snprintf(buf.get(), size, format.c_str(), args...);
  return std::string(buf.get(),
                     buf.get() + size - 1); // We don't want the '\0' inside
}

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

std::array<uint8_t, 4> to_network_byte_array(uint32_t number);
std::array<uint8_t, 2> to_network_byte_array(uint16_t number);

template <typename N> N to_number(std::vector<uint8_t> bytes) {
  size_t max_size = sizeof(N) / sizeof(uint8_t);
  if (bytes.size() != max_size) {
    throw std::invalid_argument(
        string_format("The given vector is not the right size! Vector: %d | "
                      "Bytes in number: %d",
                      bytes.size(), max_size));
  }
  N value = 0;
  for (size_t i = 0; i < max_size; i++) {
    uint8_t shift = 8 * (max_size - i - 1);
    value += bytes[i] << shift;
  }
  return value;
}
} // namespace tinydhcpd