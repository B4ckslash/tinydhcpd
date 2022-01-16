#pragma once

#include <cstdint>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <netinet/in.h>

#include <vector>

#include "datagram.hpp"

namespace tinydhcpd {
struct SubnetConfiguration {
  struct in_addr subnet_address;
  struct in_addr range_start;
  struct in_addr range_end;
  struct in_addr netmask;

  std::vector<std::pair<struct ether_addr, struct in_addr>> fixed_hosts;
  std::map<OptionTag, std::vector<uint8_t>> defined_options;
};
} // namespace tinydhcpd
