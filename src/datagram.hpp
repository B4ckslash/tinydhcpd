#pragma once

#include <iostream>
#include <vector>

namespace tinydhcpd
{
    struct DhcpOption
    {
        uint8_t tag;
        uint8_t length;
        std::vector<uint8_t> value;
    };

    struct DhcpDatagram
    {
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

        std::vector<DhcpOption> options;

        DhcpDatagram(uint8_t *buffer, int buflen);

        std::vector<uint8_t> to_byte_vector();
    };

    uint16_t convert_network_byte_array_to_uint16(uint8_t* array);
    uint32_t convert_network_byte_array_to_uint32(uint8_t* array);
} // namespace tinydhcpd