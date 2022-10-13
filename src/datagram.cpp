#include "datagram.hpp"

#include "log/logger.hpp"
#include "string-format.hpp"

namespace tinydhcpd {
const size_t OPCODE_OFFSET = 0;
const size_t HWADDR_TYPE_OFFSET = 1;
const size_t HWADDR_LENGTH_OFFSET = 2;
const size_t NR_HOPS_OFFSET = 3;
const size_t TRANSACTION_ID_OFFSET = 4;
const size_t SECS_PASSED_OFFSET = 8;
const size_t FLAGS_OFFSET = 10;
const size_t CLIENT_IP_OFFSET = 12;
const size_t ASSIGNED_IP_OFFSET = 16;
const size_t SERVER_IP_OFFSET = 20;
const size_t RELAY_AGENT_IP_OFFSET = 24;
const size_t CLIENT_HWADDR_OFFSET = 28;
const size_t SERVER_HOSTNAME_OFFSET = 44;
const size_t BOOT_FILE_NAME_OFFSET = 108;
const size_t MAGIC_COOKIE_OFFSET = 236;
const size_t OPTIONS_OFFSET = 240;

const uint32_t DHCP_MAGIC_COOKIE = 0x63825363;

const std::unordered_map<OptionTag, uint8_t> predefined_option_lengths = {
    {OptionTag::PAD, 0},
    {OptionTag::DHCP_MESSAGE_TYPE, 1},
    {OptionTag::SUBNET_MASK, 4},
    {OptionTag::TIME_OFFSET, 4},
    {OptionTag::REQUESTED_IP_ADDRESS, 4},
    {OptionTag::OPTIONS_END, 0},
};

DhcpDatagram DhcpDatagram::from_buffer(uint8_t *buffer, size_t buflen) {
  DhcpDatagram datagram;
  datagram._opcode = buffer[OPCODE_OFFSET];
  datagram._hwaddr_type = buffer[HWADDR_TYPE_OFFSET];
  datagram._hwaddr_len = buffer[HWADDR_LENGTH_OFFSET];

  datagram._transaction_id =
      convert_network_byte_array_to_uint32(buffer + TRANSACTION_ID_OFFSET);
  datagram._secs_passed =
      convert_network_byte_array_to_uint16(buffer + SECS_PASSED_OFFSET);
  datagram._flags = to_number<uint16_t>(buffer + FLAGS_OFFSET);

  datagram._client_ip = to_number<uint32_t>(buffer + CLIENT_IP_OFFSET);
  datagram._assigned_ip = to_number<uint32_t>(buffer + ASSIGNED_IP_OFFSET);
  datagram._server_ip = to_number<uint32_t>(buffer + SERVER_IP_OFFSET);

  std::copy(buffer + CLIENT_HWADDR_OFFSET, buffer + SERVER_HOSTNAME_OFFSET - 1,
            datagram._hw_addr.begin());
  uint32_t cookie = ntohl(*(uint32_t *)(buffer + MAGIC_COOKIE_OFFSET));
  if (cookie != DHCP_MAGIC_COOKIE) {
    LOG_DEBUG(string_format("DHCP cookie: got %x | expected %x", cookie,
                            DHCP_MAGIC_COOKIE));
    LOG_WARN("Not a DHCP message!");
  }

  datagram._options =
      parse_options(buffer + OPTIONS_OFFSET, buflen - OPTIONS_OFFSET);
  return datagram;
}

std::unordered_map<OptionTag, std::vector<uint8_t>>
DhcpDatagram::parse_options(const uint8_t *options_buffer, size_t buffer_size) {
  std::unordered_map<OptionTag, std::vector<uint8_t>> parsed_options;
  while (options_buffer < (options_buffer + buffer_size)) {
    OptionTag tag = static_cast<OptionTag>(*options_buffer++);
    if (predefined_option_lengths.contains(tag) &&
        predefined_option_lengths.at(tag) == 0) {
      if (tag == OptionTag::OPTIONS_END) {
        break;
      }
      continue;
    }

    uint8_t option_length = *(options_buffer++);
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
  std::vector<uint8_t> bytes;
  bytes.push_back(_opcode);
  bytes.push_back(_hwaddr_type);
  bytes.push_back(_hwaddr_len);
  bytes.push_back(0x0); // hops
  convert_number_to_network_byte_array_and_push(bytes, _transaction_id);
  convert_number_to_network_byte_array_and_push(bytes, _secs_passed);
  convert_number_to_byte_array_and_push(bytes, _flags);
  convert_number_to_byte_array_and_push(bytes, _client_ip);
  convert_number_to_byte_array_and_push(bytes, _assigned_ip);
  convert_number_to_byte_array_and_push(bytes, _server_ip);
  convert_number_to_byte_array_and_push(bytes, _relay_agent_ip);
  std::copy(_hw_addr.begin(), _hw_addr.end(), std::back_inserter(bytes));
  std::fill_n(std::back_inserter(bytes), 192, 0x0); // server name & boot file
  bytes.push_back((DHCP_MAGIC_COOKIE & (0xff << 24)) >> 24);
  bytes.push_back((DHCP_MAGIC_COOKIE & (0xff << 16)) >> 16);
  bytes.push_back((DHCP_MAGIC_COOKIE & (0xff << 8)) >> 8);
  bytes.push_back(DHCP_MAGIC_COOKIE & 0xff);
  for (auto &entry : _options) {
    bytes.push_back(static_cast<uint8_t>(entry.first));
    bytes.push_back(static_cast<uint8_t>(entry.second.size()));
    std::copy(entry.second.begin(), entry.second.end(),
              std::back_inserter(bytes));
  }
  bytes.push_back(static_cast<uint8_t>(OptionTag::OPTIONS_END));

  return bytes;
}
} // namespace tinydhcpd
