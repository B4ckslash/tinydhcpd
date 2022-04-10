#include "configuration.hpp"

#include <arpa/inet.h>
#include <libconfig.h++>
#include <netinet/ether.h>

#include "bytemanip.hpp"
#include "log/logger.hpp"
#include "string-format.hpp"

namespace tinydhcpd {
void parse_configuration(ProgramConfiguration &optval) {
  using ::libconfig::Setting, ::libconfig::Config;
  Config configuration;

  LOG_INFO("Reading config file");
  configuration.readFile(optval.confpath);
  LOG_TRACE("Parsing configuration...");

  std::string config_listen_address;
  if (!configuration.lookupValue(LISTEN_ADDRESS_KEY, config_listen_address) &&
      !configuration.lookupValue(INTERFACE_KEY, optval.interface)) {
    throw std::invalid_argument(
        "One of (listen-address|interface) must be given!");
  }

  configuration.lookupValue(LEASE_FILE_KEY, optval.lease_file_path);

  if (!config_listen_address.empty()) {
    inet_aton(config_listen_address.c_str(), &(optval.address));
  }

  if (optval.interface.empty()) {
    optval.interface =
        static_cast<std::string>(configuration.lookup(INTERFACE_KEY));
  }

  if (!configuration.exists("subnet")) {
    throw std::invalid_argument("No subnet declaration found!");
  }

  Setting &subnet_parsed_cfg = configuration.lookup("subnet");
  SubnetConfiguration subnet_cfg;
  std::string config_net_addr, config_netmask, config_range_start,
      config_range_end;
  if (!subnet_parsed_cfg.lookupValue(NET_ADDRESS_KEY, config_net_addr) ||
      !subnet_parsed_cfg.lookupValue(NETMASK_KEY, config_netmask) ||
      !subnet_parsed_cfg.lookupValue(RANGE_START_KEY, config_range_start) ||
      !subnet_parsed_cfg.lookupValue(RANGE_END_KEY, config_range_end)) {
    throw std::invalid_argument("Invalid subnet declaration!");
  }
  inet_aton(config_net_addr.c_str(), &(subnet_cfg.subnet_address));
  inet_aton(config_netmask.c_str(), &(subnet_cfg.netmask));
  inet_aton(config_range_start.c_str(), &(subnet_cfg.range_start));
  inet_aton(config_range_end.c_str(), &(subnet_cfg.range_end));

  check_net_range(subnet_cfg);

  if (subnet_parsed_cfg.exists(HOSTS_KEY)) {
    parse_hosts(subnet_parsed_cfg, subnet_cfg);
  }

  if (subnet_parsed_cfg.exists(OPTIONS_KEY)) {
    parse_options(subnet_parsed_cfg, subnet_cfg);
  }

  // convert to host long because the bytes are sent as-is, but
  // inet_aton puts them into network order (i.e. big endian)
  subnet_cfg.defined_options[OptionTag::SUBNET_MASK] =
      to_byte_vector(ntohl(subnet_cfg.netmask.s_addr));

  uint32_t lease_time_seconds = DEFAULT_LEASE_TIME;
  subnet_parsed_cfg.lookupValue(LEASE_TIME_KEY, lease_time_seconds);
  subnet_cfg.defined_options[OptionTag::LEASE_TIME] =
      to_byte_vector(lease_time_seconds);
  subnet_cfg.lease_time_seconds = lease_time_seconds;
  optval.subnet_config = subnet_cfg;
}

void check_net_range(SubnetConfiguration &cfg) {
  in_addr_t netmasked_network_address =
      cfg.subnet_address.s_addr & cfg.netmask.s_addr;
  if ((cfg.range_start.s_addr & cfg.netmask.s_addr) !=
      netmasked_network_address) {
    throw std::invalid_argument(string_format(
        "Either the range start or the net address is invalid! Netaddr: %s | "
        "Range start %s",
        inet_ntoa(cfg.subnet_address), inet_ntoa(cfg.range_start)));
  }
  if ((cfg.range_end.s_addr & cfg.netmask.s_addr) !=
      netmasked_network_address) {
    throw std::invalid_argument(
        string_format("Either the range end or the net address is invalid! "
                      "Netaddr: %s | Range end %s",
                      inet_ntoa(cfg.subnet_address), inet_ntoa(cfg.range_end)));
  }
}

void parse_hosts(libconfig::Setting &subnet_cfg_block,
                 SubnetConfiguration &subnet_cfg) {
  using ::libconfig::Setting, ::libconfig::SettingIterator;

  Setting &hosts_config = subnet_cfg_block.lookup(HOSTS_KEY);
  for (SettingIterator iter = hosts_config.begin(); iter != hosts_config.end();
       iter++) {
    Setting &currentGroup = *iter;
    std::string config_ether_addr, config_fixed_address;
    if (currentGroup.lookupValue(HOSTS_TYPE_ETHER_KEY, config_ether_addr) &&
        currentGroup.lookupValue(HOSTS_FIXED_ADDRESS_KEY,
                                 config_fixed_address)) {
      struct ether_addr *parsed_ether_addr =
          ether_aton(config_ether_addr.c_str());
      in_addr parsed_ip4_addr = {};
      in_addr_t netmasked_subnet_address =
          subnet_cfg.subnet_address.s_addr & subnet_cfg.netmask.s_addr;

      if (parsed_ether_addr == nullptr) {
        LOG_WARN(std::string("Invalid MAC address in hosts list: ")
                     .append(config_ether_addr));
        continue;
      }
      if (inet_aton(config_fixed_address.c_str(), &parsed_ip4_addr) == 0) {
        LOG_WARN(std::string("Invalid fixed address in hosts list: ")
                     .append(config_fixed_address));
        continue;
      }
      if ((parsed_ip4_addr.s_addr & subnet_cfg.netmask.s_addr) !=
          netmasked_subnet_address) {
        LOG_WARN(std::string("Address is not in subnet: ")
                     .append(config_fixed_address));
        continue;
      }

      subnet_cfg.fixed_hosts[*parsed_ether_addr] = parsed_ip4_addr;
    }
  }
}

void parse_options(libconfig::Setting &subnet_cfg_block,
                   SubnetConfiguration &subnet_cfg) {
  using ::libconfig::Setting, ::libconfig::SettingIterator;
  Setting &options_config = subnet_cfg_block.lookup(OPTIONS_KEY);
  for (SettingIterator options_iter = options_config.begin();
       options_iter != options_config.end(); options_iter++) {
    Setting &current_setting = *options_iter;
    std::string option_key(current_setting.getName());
    libconfig::Setting::Type valueType = current_setting.getType();

    try {
      OptionTag tag = key_tag_mapping.at(option_key);
      switch (valueType) {

      case libconfig::Setting::Type::TypeInt: {
        int config_value = current_setting;
        auto netarray = to_network_byte_array((uint16_t)config_value);
        std::vector<uint8_t> value(netarray.begin(), netarray.end());
        subnet_cfg.defined_options[tag] = value;
        std::ostringstream os;
        os << "Read option " << string_format("%x", static_cast<uint8_t>(tag))
           << " | Value: ";
        for (auto &elem : value) {
          os << string_format("%x ", elem);
        }
        LOG_TRACE(os.str());
        break;
      }

      case libconfig::Setting::Type::TypeArray: {
        std::vector<uint8_t> addresses;
        for (SettingIterator array_iter = current_setting.begin();
             array_iter != current_setting.end(); array_iter++) {
          const std::array<uint8_t, 4> &address_bytes =
              parse_ip_address(*array_iter);
          addresses.insert(addresses.end(), address_bytes.begin(),
                           address_bytes.end());
        }
        subnet_cfg.defined_options[tag] = addresses;
        std::ostringstream os;
        os << "Read option " << string_format("%x", static_cast<uint8_t>(tag))
           << " | Value: ";
        for (auto &elem : addresses) {
          os << string_format("%d ", elem);
        }
        LOG_TRACE(os.str());
        break;
      }

      default:
        const std::array<uint8_t, 4> &address_bytes =
            parse_ip_address(current_setting);
        std::vector<uint8_t> address(address_bytes.begin(),
                                     address_bytes.end());
        subnet_cfg.defined_options[tag] = address;
        std::ostringstream os;
        os << "Read option " << string_format("%x", static_cast<uint8_t>(tag))
           << " | Value: ";
        for (auto &elem : address) {
          os << string_format("%d ", elem);
        }
        LOG_TRACE(os.str());
        break;
      }
    } catch (std::out_of_range &oor) {
      LOG_WARN(std::string("Unknown option: ").append(option_key));
      continue;
    }
  }
}

std::array<uint8_t, 4> parse_ip_address(const char *address_string) {
  in_addr_t address = inet_network(address_string);
  std::array<uint8_t, 4> address_bytes = to_byte_array<>(address);
  return address_bytes;
}
} // namespace tinydhcpd
