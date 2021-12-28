#include "datagram.hpp"

#include <arpa/inet.h>

#define OPCODE_OFFSET 0
#define HWADDR_TYPE_OFFSET 1
#define HWADDR_LENGTH_OFFSET 2
#define NR_HOPS_OFFSET 3
#define TRANSACTION_ID_OFFSET 4
#define SECS_PASSED_OFFSET 8
#define FLAGS_OFFSET 10
#define CLIENT_IP_OFFSET 12
#define ASSIGNED_IP_OFFSET 16
#define SERVER_IP_OFFSET 20
#define RELAY_AGENT_IP_OFFSET 24
#define CLIENT_HWADDR_OFFSET 28
#define SERVER_HOSTNAME_OFFSET 44
#define BOOT_FILE_NAME_OFFSET 108
#define MAGIC_COOKIE_OFFSET 236
#define OPTIONS_OFFSET 240

#define DHCP_MAGIC_COOKIE 0x63825363

namespace tinydhcpd
{
    DhcpDatagram::DhcpDatagram(uint8_t* buffer, int buflen)
    {
        using namespace tinydhcpd;

        opcode = buffer[OPCODE_OFFSET];
        hwaddr_type = buffer[HWADDR_TYPE_OFFSET];
        hwaddr_len = buffer[HWADDR_LENGTH_OFFSET];

        transaction_id = convert_network_byte_array_to_uint32(buffer + TRANSACTION_ID_OFFSET);
        secs_passed = convert_network_byte_array_to_uint16(buffer + SECS_PASSED_OFFSET);
        flags = convert_network_byte_array_to_uint16(buffer + FLAGS_OFFSET);

        client_ip = convert_network_byte_array_to_uint32(buffer + CLIENT_IP_OFFSET);
        assigned_ip = convert_network_byte_array_to_uint32(buffer + ASSIGNED_IP_OFFSET);
        server_ip = convert_network_byte_array_to_uint32(buffer + SERVER_IP_OFFSET);

        std::copy(buffer + CLIENT_HWADDR_OFFSET, buffer + SERVER_HOSTNAME_OFFSET - 1, hw_addr);
        uint32_t cookie = ntohl(*(uint32_t*)(buffer + MAGIC_COOKIE_OFFSET));
        if (cookie != DHCP_MAGIC_COOKIE)
        {
            std::cout << cookie << " expected " << DHCP_MAGIC_COOKIE << std::endl;
            throw std::runtime_error("Not a DHCP message!");
        }

        std::vector<uint8_t> options(buffer + OPTIONS_OFFSET, buffer + buflen - OPTIONS_OFFSET);
    }

    std::vector<uint8_t> DhcpDatagram::to_byte_vector()
    {
        return std::vector<uint8_t>();
    }

    uint16_t convert_network_byte_array_to_uint16(uint8_t* array)
    {
        return ntohs(*(uint16_t*)array);
    }

    uint32_t convert_network_byte_array_to_uint32(uint8_t* array)
    {
        return ntohl(*(uint32_t*)array);
    }
} // namespace tinydhcpd
