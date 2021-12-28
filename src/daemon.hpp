#pragma once

#include "socket.hpp"

#include <netinet/in.h>

#include <iostream>
#include <csignal>

#include "socket_observer.hpp"

namespace tinydhcpd
{
    extern volatile std::sig_atomic_t last_signal;
    class Daemon : SocketObserver
    {
    private:
        Socket socket;

    public:
        Daemon(const struct in_addr& listen_address);
        Daemon(const std::string& iface_name);
        Daemon(const struct in_addr& adress, const std::string& iface_name);
        virtual ~Daemon() {}
        virtual void handle_recv(DhcpDatagram& datagram) override;
    };

} // namespace tinydhcpd