#include "configuration.hpp"

#include <arpa/inet.h>
#include <netinet/ether.h>
#include <libconfig.h++>

#include <string>

#include "utils.hpp"

namespace tinydhcpd
{
    void parse_configuration(option_values& optval)
    {
        using ::libconfig::Setting, ::libconfig::Config;
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

        if (subnet_parsed_cfg.exists(HOSTS_KEY))
        {
            parse_hosts(subnet_parsed_cfg, subnet_cfg);
        }

        //TODO parse options

        optval.subnet_config = subnet_cfg;
    }

    void check_net_range(SubnetConfiguration& cfg)
    {
        in_addr_t netmasked_network_address = cfg.subnet_address.s_addr & cfg.netmask.s_addr;
        //TODO fix logging
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

    void parse_hosts(libconfig::Setting& subnet_block, SubnetConfiguration& subnet_cfg)
    {
        using ::libconfig::Setting, ::libconfig::SettingIterator;

        Setting& hosts_config = subnet_block.lookup(HOSTS_KEY);
        for (SettingIterator iter = hosts_config.begin(); iter != hosts_config.end(); iter++)
        {
            Setting& currentGroup = *iter;
            std::string config_ether_addr, config_fixed_address;
            if (currentGroup.lookupValue(HOSTS_ETHER_HWADDR_KEY, config_ether_addr) && currentGroup.lookupValue(HOSTS_FIXED_ADDRESS_KEY, config_fixed_address))
            {
                ether_addr* parsed_ether_addr = ether_aton(config_ether_addr.c_str());
                in_addr parsed_ip4_addr = {};
                in_addr_t netmasked_subnet_address = subnet_cfg.subnet_address.s_addr & subnet_cfg.netmask.s_addr;

                if (parsed_ether_addr == nullptr)
                {
                    std::cerr << "Invalid MAC address in hosts list: " << config_ether_addr << std::endl;
                    continue;
                }
                if (inet_aton(config_fixed_address.c_str(), &parsed_ip4_addr) == 0)
                {
                    std::cerr << "Invalid fixed address in hosts list: " << config_fixed_address << std::endl;
                    continue;
                }
                if((parsed_ip4_addr.s_addr & subnet_cfg.netmask.s_addr) != netmasked_subnet_address)
                {
                    std::cerr << "Address is not in subnet: " << config_fixed_address << std::endl;
                    continue;
                }

                std::cout << "Read pair of " << config_ether_addr << " & " << config_fixed_address << std::endl;
                subnet_cfg.fixed_hosts.push_back({ *parsed_ether_addr, parsed_ip4_addr });
            }
        }
    }
} // namespace tinydhcpd