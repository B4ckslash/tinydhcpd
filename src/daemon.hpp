#pragma once

#include "socket.hpp"

#include <iostream>

#include <libconfig.h++>

#include "socket_observer.hpp"

namespace tinydhcpd
{
    class Daemon : SocketObserver
    {
    private:
        Socket socket;

    public:
        Daemon(uint32_t listen_address);
        Daemon(const std::string &iface_name);
        virtual ~Daemon() {}
        virtual void handle_recv(dhcp_datagram &datagram) override;
    };

} // namespace tinydhcpd