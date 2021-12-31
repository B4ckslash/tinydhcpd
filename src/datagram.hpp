#pragma once

#include <iostream>
#include <vector>

namespace tinydhcpd
{
    enum struct OptionTag : uint8_t
    {
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
        STATIC_ROUTES = 33
    };

    struct DhcpOption
    {
        OptionTag tag;
        size_t length;
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

        DhcpDatagram(uint8_t* buffer, int buflen);

        std::vector<uint8_t> to_byte_vector();
    };

    uint16_t convert_network_byte_array_to_uint16(uint8_t* array);
    uint32_t convert_network_byte_array_to_uint32(uint8_t* array);
} // namespace tinydhcpd