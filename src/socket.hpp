#ifndef SOCKET_HPP
#define SOCKET_HPP
#include <iostream>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>

#define PORT 67


namespace tinydhcpd
{
    struct dhcp_datagram
    {
        uint8_t opcode;
        uint8_t hwaddr_type;
        uint8_t hwaddr_len;

        uint8_t transaction_id[4];
        uint8_t secs_passed[2];
        uint8_t flags[2];

        uint8_t client_ip[4];
        uint8_t my_ip[4];
        uint8_t next_server_ip[4];

        uint8_t hw_addr[16];
        std::string server_name;

        std::string boot_file_name;
        std::vector<uint8_t> options;
    };

    class Socket
    {
    private:
    public:
        Socket(std::string if_name){}
        Socket(uint8_t listen_address[4]){}
    };

    dhcp_datagram recv_datagram();
    void send_datagram(dhcp_datagram &datagram);

} // namespace tinydhcpd
#endif