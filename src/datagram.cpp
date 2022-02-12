#include "datagram.hpp"

#include <arpa/inet.h>

#include <cstdint>
#include <map>
#include <stdexcept>

namespace tinydhcpd {
const int OPCODE_OFFSET = 0;
const int HWADDR_TYPE_OFFSET = 1;
const int HWADDR_LENGTH_OFFSET = 2;
const int NR_HOPS_OFFSET = 3;
const int TRANSACTION_ID_OFFSET = 4;
const int SECS_PASSED_OFFSET = 8;
const int FLAGS_OFFSET = 10;
const int CLIENT_IP_OFFSET = 12;
const int ASSIGNED_IP_OFFSET = 16;
const int SERVER_IP_OFFSET = 20;
const int RELAY_AGENT_IP_OFFSET = 24;
const int CLIENT_HWADDR_OFFSET = 28;
const int SERVER_HOSTNAME_OFFSET = 44;
const int BOOT_FILE_NAME_OFFSET = 108;
const int MAGIC_COOKIE_OFFSET = 236;
const int OPTIONS_OFFSET = 240;

const uint32_t DHCP_MAGIC_COOKIE = 0x63825363;

const std::map<OptionTag, uint8_t> predefined_option_lengths = {
    {OptionTag::PAD, 0},
    {OptionTag::DHCP_MESSAGE_TYPE, 1},
    {OptionTag::SUBNET_MASK, 4},
    {OptionTag::TIME_OFFSET, 4},
    {OptionTag::REQUESTED_IP_ADDRESS, 4},
    {OptionTag::OPTIONS_END, 0},
};

DhcpDatagram DhcpDatagram::from_buffer(uint8_t *buffer, int buflen) {
  DhcpDatagram datagram;
  datagram.opcode = buffer[OPCODE_OFFSET];
  datagram.hwaddr_type = buffer[HWADDR_TYPE_OFFSET];
  datagram.hwaddr_len = buffer[HWADDR_LENGTH_OFFSET];

  datagram.transaction_id =
      convert_network_byte_array_to_uint32(buffer + TRANSACTION_ID_OFFSET);
  datagram.secs_passed =
      convert_network_byte_array_to_uint16(buffer + SECS_PASSED_OFFSET);
  datagram.flags = *reinterpret_cast<uint16_t *>(buffer + FLAGS_OFFSET);

  datagram.client_ip =
      convert_network_byte_array_to_uint32(buffer + CLIENT_IP_OFFSET);
  datagram.assigned_ip =
      convert_network_byte_array_to_uint32(buffer + ASSIGNED_IP_OFFSET);
  datagram.server_ip =
      convert_network_byte_array_to_uint32(buffer + SERVER_IP_OFFSET);

  std::copy(buffer + CLIENT_HWADDR_OFFSET, buffer + SERVER_HOSTNAME_OFFSET - 1,
            datagram.hw_addr.begin());
  uint32_t cookie = ntohl(*(uint32_t *)(buffer + MAGIC_COOKIE_OFFSET));
  if (cookie != DHCP_MAGIC_COOKIE) {
    std::cout << cookie << " expected " << DHCP_MAGIC_COOKIE << std::endl;
    throw std::runtime_error("Not a DHCP message!");
  }

  datagram.options =
      parse_options(buffer + OPTIONS_OFFSET, buflen - OPTIONS_OFFSET);
  return datagram;
}

std::map<OptionTag, std::vector<uint8_t>>
DhcpDatagram::parse_options(const uint8_t *options_buffer, size_t buffer_size) {
  std::map<OptionTag, std::vector<uint8_t>> parsed_options;
  while (options_buffer < (options_buffer + buffer_size)) {
    OptionTag tag = static_cast<OptionTag>(*options_buffer++);
    if (predefined_option_lengths.contains(tag) &&
        predefined_option_lengths.at(tag) == 0) {
      if (tag == OptionTag::OPTIONS_END) {
        break;
      }
      continue;
    }

    uint8_t option_length = *options_buffer++;
    if (predefined_option_lengths.contains(tag) &&
        predefined_option_lengths.at(tag) != option_length) {
      throw std::invalid_argument(
          string_format("Invalid option length! Tag: %u | Legal length: %u | "
                        "Actual length: %u",
                        static_cast<uint8_t>(tag),
                        predefined_option_lengths.at(tag), option_length));
    }
    std::vector<uint8_t> value(options_buffer, options_buffer + option_length);
    parsed_options[tag] = value;
    options_buffer += option_length;
  }
  return parsed_options;
}

std::vector<uint8_t> DhcpDatagram::to_byte_vector() {
  std::vector<uint8_t> byte_vector;
  byte_vector.push_back(opcode);
  byte_vector.push_back(hwaddr_type);
  byte_vector.push_back(hwaddr_len);
  byte_vector.push_back(0x0); // hops
  convert_number_to_byte_array_and_push(byte_vector, transaction_id);
  convert_number_to_network_byte_array_and_push(byte_vector, secs_passed);
  byte_vector.push_back(0x0); // flags
  byte_vector.push_back(0x0); // flags
  convert_number_to_byte_array_and_push(byte_vector, client_ip);
  convert_number_to_byte_array_and_push(byte_vector, assigned_ip);
  convert_number_to_byte_array_and_push(byte_vector, server_ip);
  convert_number_to_byte_array_and_push(byte_vector, relay_agent_ip);
  std::copy(hw_addr.begin(), hw_addr.end(), std::back_inserter(byte_vector));
  std::fill_n(std::back_inserter(byte_vector), 192, 0x0);
  byte_vector.push_back((DHCP_MAGIC_COOKIE & (0xff << 24)) >> 24);
  byte_vector.push_back((DHCP_MAGIC_COOKIE & (0xff << 16)) >> 16);
  byte_vector.push_back((DHCP_MAGIC_COOKIE & (0xff << 8)) >> 8);
  byte_vector.push_back(DHCP_MAGIC_COOKIE & 0xff);
  for (auto &entry : options) {
    byte_vector.push_back(static_cast<uint8_t>(entry.first));
    byte_vector.push_back(static_cast<uint8_t>(entry.second.size()));
    std::copy(entry.second.begin(), entry.second.end(),
              std::back_inserter(byte_vector));
  }
  byte_vector.push_back(static_cast<uint8_t>(OptionTag::OPTIONS_END));

  return byte_vector;
}

uint16_t convert_network_byte_array_to_uint16(uint8_t *array) {
  return ntohs(*(uint16_t *)array);
}

uint32_t convert_network_byte_array_to_uint32(uint8_t *array) {
  return ntohl(*(uint32_t *)array);
}

} // namespace tinydhcpd
