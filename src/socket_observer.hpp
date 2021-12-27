#pragma once

#include "datagram.hpp"

namespace tinydhcpd
{
    class SocketObserver
    {
    private:
    public:
        virtual ~SocketObserver() {};

        virtual void handle_recv(tinydhcpd::dhcp_datagram &datagram) = 0;
    };
} // namespace tinydhcpd
