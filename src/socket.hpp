#pragma once

#include <sys/epoll.h>

#include <iostream>

#include "datagram.hpp"
#include "socket_observer.hpp"

#define PORT 67
#define MAX_EVENTS 10

namespace tinydhcpd
{
    class Socket
    {
    private:
        int socket_fd, epoll_fd;
        struct epoll_event ev, events[MAX_EVENTS];
        SocketObserver &observer;
        void die(std::string error_msg);
        void create_socket();
        void setup_epoll();

    public:
        Socket(const std::string if_name, SocketObserver &observer);
        Socket(uint32_t listen_address, SocketObserver &observer);
        ~Socket() noexcept;
        void send_datagram(dhcp_datagram &datagram);
        void recv_loop();
    };

} // namespace tinydhcpd