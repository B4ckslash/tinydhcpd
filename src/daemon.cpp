#include "daemon.hpp"

namespace tinydhcpd
{
    Daemon::Daemon(uint8_t listen_address[4]) : socket{listen_address}
    {
        std::printf("listen address: %u.%u.%u.%u\n", listen_address[0], listen_address[1], listen_address[2], listen_address[3]);
    }

    Daemon::Daemon(const std::string& iface_name) : socket{iface_name}
    {
        std::cout << "bind to iface: " << iface_name << std::endl;
    }
} // namespace tinydhcpd
