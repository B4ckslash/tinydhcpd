#include <netinet/in.h>
#include <libconfig.h++>

#include <iostream>

#include "subnet_config.hpp"

namespace tinydhcpd
{
    const std::string LISTEN_ADDRESS_KEY = "listen-address";
    const std::string INTERFACE_KEY = "interface";
    const std::string NET_ADDRESS_KEY = "net-address";
    const std::string NETMASK_KEY = "netmask";
    const std::string RANGE_START_KEY = "range-start";
    const std::string RANGE_END_KEY = "range-end";
    const std::string OPTIONS_KEY = "options";
    const std::string HOSTS_KEY = "hosts";

    const std::string HOSTS_ETHER_HWADDR_KEY = "ether";
    const std::string HOSTS_FIXED_ADDRESS_KEY = "fixed-address";

    struct option_values {
        struct in_addr address;
        std::string interface;
        std::string confpath;
        tinydhcpd::SubnetConfiguration subnet_config;
    };

    void parse_configuration(option_values& optval);
    void check_net_range(SubnetConfiguration& cfg);
    void parse_hosts(libconfig::Setting& subnet_block, SubnetConfiguration& subnet_cfg);
} // namespace tinydhcpd
