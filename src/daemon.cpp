#include "daemon.hpp"


namespace tinydhcpd
{
    Daemon::Daemon(const struct in_addr &listen_address) : socket{listen_address, *this}
    {
    }

    Daemon::Daemon(const std::string &iface_name) : socket{iface_name, *this}
    {
        std::cout << "bind to iface: " << iface_name << std::endl;
    }

    void Daemon::handle_recv(dhcp_datagram &datagram)
    {
    }
} // namespace tinydhcpd
