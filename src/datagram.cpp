#include "datagram.hpp"
#include "utils.hpp"

#include <arpa/inet.h>

#include <map>
#include <stdexcept>


namespace tinydhcpd
{
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

    const int DHCP_MAGIC_COOKIE = 0x63825363;

    const std::map<OptionTag, uint8_t> predefined_option_lengths = {
        {OptionTag::PAD, 0}, {OptionTag::DHCP_MESSAGE_TYPE, 1}, {OptionTag::SUBNET_MASK, 4},
        {OptionTag::TIME_OFFSET, 4}, {OptionTag::REQUESTED_IP_ADDRESS, 4},
        {OptionTag::OPTIONS_END, 0},
    };

    DhcpDatagram::DhcpDatagram(uint8_t* buffer, int buflen)
    {
        using namespace tinydhcpd;

        opcode = buffer[OPCODE_OFFSET];
        hwaddr_type = buffer[HWADDR_TYPE_OFFSET];
        hwaddr_len = buffer[HWADDR_LENGTH_OFFSET];

        transaction_id = convert_network_byte_array_to_uint32(buffer + TRANSACTION_ID_OFFSET);
        secs_passed = convert_network_byte_array_to_uint16(buffer + SECS_PASSED_OFFSET);
        flags = convert_network_byte_array_to_uint16(buffer + FLAGS_OFFSET);

        client_ip = convert_network_byte_array_to_uint32(buffer + CLIENT_IP_OFFSET);
        assigned_ip = convert_network_byte_array_to_uint32(buffer + ASSIGNED_IP_OFFSET);
        server_ip = convert_network_byte_array_to_uint32(buffer + SERVER_IP_OFFSET);

        std::copy(buffer + CLIENT_HWADDR_OFFSET, buffer + SERVER_HOSTNAME_OFFSET - 1, hw_addr);
        uint32_t cookie = ntohl(*(uint32_t*)(buffer + MAGIC_COOKIE_OFFSET));
        if (cookie != DHCP_MAGIC_COOKIE)
        {
            std::cout << cookie << " expected " << DHCP_MAGIC_COOKIE << std::endl;
            throw std::runtime_error("Not a DHCP message!");
        }

        options = parse_options(buffer + OPTIONS_OFFSET, buflen - OPTIONS_OFFSET);
    }

    std::vector<DhcpOption> DhcpDatagram::parse_options(const uint8_t* options_buffer, size_t buffer_size)
    {
        std::vector<DhcpOption> parsed_options;
        while (options_buffer < (options_buffer + buffer_size))
        {
            OptionTag tag = static_cast<OptionTag>(*options_buffer++);
            if (predefined_option_lengths.contains(tag) && predefined_option_lengths.at(tag) == 0)
            {
                if (tag == OptionTag::OPTIONS_END)
                {
                    break;
                }
                continue;
            }

            uint8_t option_length = *options_buffer++;
            if (predefined_option_lengths.contains(tag) && predefined_option_lengths.at(tag) != option_length)
            {
                throw std::invalid_argument(string_format("Invalid option length! Tag: %u | Legal length: %u | Actual length: %u",
                    static_cast<uint8_t>(tag), predefined_option_lengths.at(tag), option_length));
            }
            std::vector<uint8_t> value(options_buffer, options_buffer + option_length);
            parsed_options.emplace_back(tag, option_length, value);
            options_buffer += option_length;
        }
        return parsed_options;
    }

    std::vector<uint8_t> DhcpDatagram::to_byte_vector()
    {
        return std::vector<uint8_t>();
    }

    uint16_t convert_network_byte_array_to_uint16(uint8_t* array)
    {
        return ntohs(*(uint16_t*)array);
    }

    uint32_t convert_network_byte_array_to_uint32(uint8_t* array)
    {
        return ntohl(*(uint32_t*)array);
    }
} // namespace tinydhcpd
