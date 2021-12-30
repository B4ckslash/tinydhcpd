#pragma once

#include <netinet/in.h>
#include <linux/if_packet.h>

#include <vector>

#include "datagram.hpp"

namespace tinydhcpd
{
    struct SubnetConfiguration
    {
        struct in_addr subnet_address;
        struct in_addr range_start;
        struct in_addr range_end; 
        struct in_addr netmask;

        std::vector<std::pair<struct sockaddr_ll, struct in_addr>> fixed_hosts;
        std::vector<DhcpOption> defined_options;
    };
} // namespace tinydhcpd
