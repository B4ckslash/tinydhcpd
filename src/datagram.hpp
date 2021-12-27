#pragma once

#include <iostream>
#include <vector>

namespace tinydhcpd
{
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

        std::vector<uint8_t> options;

        DhcpDatagram(uint8_t *buffer, int buflen);
    };
} // namespace tinydhcpd
