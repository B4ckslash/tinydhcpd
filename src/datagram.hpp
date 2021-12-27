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

        uint8_t transaction_id[4];
        uint8_t secs_passed[2];
        uint8_t flags[2];

        uint8_t client_ip[4];
        uint8_t my_ip[4];
        uint8_t next_server_ip[4];

        uint8_t hw_addr[16];
        std::string server_name;

        std::string boot_file_name;
        std::vector<uint8_t> options;
    };
} // namespace tinydhcpd
