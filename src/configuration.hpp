#include <libconfig.h++>
#include <netinet/in.h>

#include <iostream>
#include <map>

#include "datagram.hpp"
#include "subnet_config.hpp"

namespace tinydhcpd {
const std::string LISTEN_ADDRESS_KEY = "listen-address";
const std::string INTERFACE_KEY = "interface";
const std::string NET_ADDRESS_KEY = "net-address";
const std::string NETMASK_KEY = "netmask";
const std::string RANGE_START_KEY = "range-start";
const std::string RANGE_END_KEY = "range-end";
const std::string OPTIONS_KEY = "options";
const std::string HOSTS_KEY = "hosts";
const std::string LEASE_FILE_KEY = "lease-file";

const std::string HOSTS_TYPE_ETHER_KEY = "ether";
const std::string HOSTS_FIXED_ADDRESS_KEY = "fixed-address";

const std::string OPTIONS_ROUTER_KEY = "routers";
const std::string OPTIONS_DNS_SERVERS_KEY = "domain-name-servers";

const std::map<std::string, OptionTag> key_tag_mapping = {
    {OPTIONS_ROUTER_KEY, OptionTag::ROUTERS},
    {OPTIONS_DNS_SERVERS_KEY, OptionTag::DNS_SERVER}};

struct ProgramConfiguration {
  struct in_addr address;
  std::string interface;
  std::string confpath;
  std::string lease_file_path;
  tinydhcpd::SubnetConfiguration subnet_config;
};

void parse_configuration(ProgramConfiguration &optval);
void check_net_range(SubnetConfiguration &cfg);
void parse_hosts(libconfig::Setting &subnet_block,
                 SubnetConfiguration &subnet_cfg);
void parse_options(libconfig::Setting &subnet_block,
                   SubnetConfiguration &subnet_cfg);
std::array<uint8_t, 4> parse_ip_address(const char *address_string);
} // namespace tinydhcpd
