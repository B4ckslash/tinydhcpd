#pragma once

#include <map>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <vector>

namespace tinydhcpd {
enum struct OptionTag : uint8_t;

struct SubnetConfiguration {
  struct in_addr subnet_address;
  struct in_addr range_start;
  struct in_addr range_end;
  struct in_addr netmask;
  uint32_t lease_time_seconds;

  std::map<struct ether_addr, struct in_addr> fixed_hosts;
  std::map<OptionTag, std::vector<uint8_t>> defined_options;
};
} // namespace tinydhcpd

// global operator overload to allow usage of ether_addr as map key
inline bool operator<(const struct ether_addr lhs,
                      const struct ether_addr rhs) {
  for (int i = 0; i < ETHER_ADDR_LEN; i++) {
    if (lhs.ether_addr_octet[i] < rhs.ether_addr_octet[i]) {
      return true;
    }
  }
  return false;
}
