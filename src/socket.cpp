#include "socket.hpp"

#include <net/if.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

namespace tinydhcpd
{
    Socket::Socket(const struct in_addr &listen_address, SocketObserver &observer) : observer(observer)
    {
        std::cout << listen_address.s_addr << std::endl;

        std::printf("listen address: %s\n", inet_ntoa(listen_address));

        create_socket();
        const struct sockaddr_in inet_socket_address = {
            .sin_family = AF_INET,
            .sin_port = htons(PORT),
            .sin_addr = listen_address};
        if (bind(socket_fd, (const sockaddr *)&inet_socket_address, sizeof(inet_socket_address)) == -1)
        {
            die("Failed to bind socket: ");
        }
    }

    Socket::Socket(const std::string iface_name, SocketObserver &observer) : observer(observer)
    {
        create_socket();
        struct ifreq ireq;
        memset(&ireq, 0, sizeof(ireq));
        snprintf(ireq.ifr_name, sizeof(ireq.ifr_name), iface_name.c_str());
        if (setsockopt(socket_fd, SOL_SOCKET, SO_BINDTODEVICE, (const void *)&ireq, sizeof(ireq)) < 0)
        {
            std::string msg("Failed to bind to interface ");
            msg.append(iface_name);
            msg.append(": ");
            die(msg);
        }
    }

    Socket::~Socket()
    {
        close(epoll_fd);
        close(socket_fd);
    }

    void Socket::die(std::string error_msg)
    {
        throw std::runtime_error(error_msg.append(strerror(errno)));
    }

    void Socket::create_socket()
    {
        socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (socket_fd == -1)
        {
            die("Failed to create socket: ");
        }
    }

    void Socket::setup_epoll()
    {
        epoll_fd = epoll_create1(0);
        if (epoll_fd == -1)
        {
            die("Failed to create epoll file descriptor: ");
        }
        ev.events = EPOLLIN;
        ev.data.fd = socket_fd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &ev) == -1)
        {
            die("Failed at epoll_ctl: ");
        }
    }
} //namespace tinydhcpd