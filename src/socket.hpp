#pragma once

#include <sys/epoll.h>
#include <netinet/in.h>

#include <iostream>
#include <queue>
#include <utility>

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
        SocketObserver& observer;
        const struct sockaddr_in listen_address;
        std::queue<std::pair<struct sockaddr, DhcpDatagram>> send_queue;
        void die(std::string error_msg);
        void create_socket();
        void bind_to_iface(std::string iface_name);
        void bind_to_address(const struct sockaddr_in& address);
        void setup_epoll();

    public:
        Socket(const std::string& iface_name, SocketObserver& observer);
        Socket(const struct in_addr& address, SocketObserver& observer);
        Socket(const struct in_addr& address, const std::string& iface_name, SocketObserver& observer);
        ~Socket()noexcept;
        void enqueue_datagram(struct sockaddr &destination, DhcpDatagram& datagram);
        void recv_loop();
    };

} // namespace tinydhcpd