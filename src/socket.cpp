#include "socket.hpp"

#include <net/if.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#include <algorithm>

#include "daemon.hpp"

#define DGRAM_SIZE 576

namespace tinydhcpd
{
    Socket::Socket(const struct in_addr& address, SocketObserver& observer) : observer(observer),
        listen_address{ .sin_family = AF_INET, .sin_port = htons(PORT), .sin_addr = address, .sin_zero = {} }
    {
        create_socket();
        bind_to_address(listen_address);
        setup_epoll();
    }

    Socket::Socket(const struct in_addr& address, const std::string& iface_name, SocketObserver& observer) : observer(observer),
        listen_address{ .sin_family = AF_INET, .sin_port = htons(PORT), .sin_addr = address, .sin_zero = {} }
    {
        create_socket();
        bind_to_iface(iface_name);
        bind_to_address(listen_address);
        setup_epoll();
    }

    Socket::Socket(const std::string& iface_name, SocketObserver& observer) : Socket({ .s_addr = INADDR_ANY }, iface_name, observer) {}

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

    void Socket::bind_to_iface(const std::string iface_name)
    {
        struct ifreq ireq = {};
        snprintf(ireq.ifr_name, sizeof(ireq.ifr_name), iface_name.c_str());
        if (setsockopt(socket_fd, SOL_SOCKET, SO_BINDTODEVICE, (const void*)&ireq, sizeof(ireq)) < 0)
        {
            std::string msg("Failed to bind to interface ");
            msg.append(iface_name);
            msg.append(": ");
            die(msg);
        }
    }

    void Socket::bind_to_address(const struct sockaddr_in& address)
    {
        if (bind(socket_fd, (const sockaddr*)&address, sizeof(address)) == -1)
        {
            die("Failed to bind socket: ");
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

    void Socket::recv_loop()
    {
        int number_ready_fds;
        uint8_t raw_data_buffer[DGRAM_SIZE];
        std::fill(raw_data_buffer, raw_data_buffer + DGRAM_SIZE, (uint8_t)0);
        while (true)
        {
            if (should_terminate)
            {
                return;
            }

            number_ready_fds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
            if (number_ready_fds == -1)
            {
                if (errno == EINTR)
                {
                    continue;
                }

                die("epoll_wait failed: ");
            }
            for (int n = 0; n < number_ready_fds; n++)
            {
                if (events[n].data.fd == socket_fd)
                {
                    recv(socket_fd, raw_data_buffer, DGRAM_SIZE, MSG_WAITALL);
                    DhcpDatagram datagram(raw_data_buffer, DGRAM_SIZE);
                    observer.handle_recv(datagram);
                    std::fill(raw_data_buffer, raw_data_buffer + DGRAM_SIZE, (uint8_t)0);
                }
            }
        }
    }

    void Socket::signal_termination()
    {
        should_terminate = true;
    }
} //namespace tinydhcpd