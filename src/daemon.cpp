#include "daemon.hpp"

#include <arpa/inet.h>

namespace tinydhcpd
{
    Daemon::Daemon(uint32_t listen_address) : socket{listen_address, *this}
    {
        struct in_addr ip_addr =
            {
                .s_addr = listen_address};

        std::printf("listen address: %s\n", inet_ntoa(ip_addr));
    }

    Daemon::Daemon(const std::string &iface_name) : socket{iface_name, *this}
    {
        std::cout << "bind to iface: " << iface_name << std::endl;
    }

    void Daemon::handle_recv(dhcp_datagram &datagram)
    {
    }
} // namespace tinydhcpd
