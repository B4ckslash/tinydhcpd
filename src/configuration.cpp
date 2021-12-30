#include "configuration.hpp"

#include <arpa/inet.h>
#include <libconfig.h++>

#include <string>

#include "utils.hpp"

namespace tinydhcpd
{
    void parseConfiguration(option_values& optval)
    {
        using namespace libconfig;
        Config configuration;

        configuration.readFile(optval.confpath);

        std::string config_listen_address;
        if (!configuration.lookupValue(LISTEN_ADDRESS_KEY, config_listen_address)
            && !configuration.lookupValue(INTERFACE_KEY, optval.interface))
        {
            throw std::invalid_argument("One of (listen-address|interface) must be given!");
        }
        if (!config_listen_address.empty())
        {
            inet_aton(config_listen_address.c_str(), &(optval.address));
        }


        if (!configuration.exists("subnet"))
        {
            throw std::invalid_argument("No subnet declaration found!");
        }

        Setting& subnet_parsed_cfg = configuration.lookup("subnet");
        SubnetConfiguration subnet_cfg;
        std::string config_net_addr, config_netmask, config_range_start, config_range_end;
        if (!subnet_parsed_cfg.lookupValue(NET_ADDRESS_KEY, config_net_addr) ||
            !subnet_parsed_cfg.lookupValue(NETMASK_KEY, config_netmask) ||
            !subnet_parsed_cfg.lookupValue(RANGE_START_KEY, config_range_start) ||
            !subnet_parsed_cfg.lookupValue(RANGE_END_KEY, config_range_end)
            )
        {
            throw std::invalid_argument("Invalid subnet declaration!");
        }
        inet_aton(config_net_addr.c_str(), &(subnet_cfg.subnet_address));
        inet_aton(config_netmask.c_str(), &(subnet_cfg.netmask));
        inet_aton(config_range_start.c_str(), &(subnet_cfg.range_start));
        inet_aton(config_range_end.c_str(), &(subnet_cfg.range_end));

        check_net_range(subnet_cfg);

        //TODO parse hosts and options

        optval.subnet_config = subnet_cfg;
    }

    void check_net_range(SubnetConfiguration& cfg)
    {
        in_addr_t netmasked_network_address = cfg.subnet_address.s_addr & cfg.netmask.s_addr;
        if ((cfg.range_start.s_addr & cfg.netmask.s_addr) != netmasked_network_address)
        {
            throw std::invalid_argument(string_format("Either the range start or the net address is invalid! Netaddr: %s | Range start %s",
                inet_ntoa(cfg.subnet_address), inet_ntoa(cfg.range_start)));
        }
        if ((cfg.range_end.s_addr & cfg.netmask.s_addr) != netmasked_network_address)
        {
            throw std::invalid_argument(string_format("Either the range end or the net address is invalid! Netaddr: %s | Range end %s",
                inet_ntoa(cfg.subnet_address), inet_ntoa(cfg.range_end)));
        }
    }

} // namespace tinydhcpd
