#include "daemon.hpp"

#include <arpa/inet.h>

#include "utils.hpp"

namespace tinydhcpd
{
    volatile std::sig_atomic_t last_signal;

    Daemon::Daemon(const struct in_addr& address, const std::string& iface_name, SubnetConfiguration& netconfig)
        try : socket{ address, iface_name, *this }, netconfig(netconfig)
    {
        socket.recv_loop();
    }
    catch (std::runtime_error& ex)
    {
        printf("%s\n", ex.what());
        exit(EXIT_FAILURE);
    }

    void Daemon::handle_recv(DhcpDatagram& datagram)
    {
        std::cout << string_format("XID: %#010x", datagram.transaction_id) << std::endl;
        std::cout << "Subnet: " << inet_ntoa(netconfig.subnet_address) << std::endl;
        std::cout << "Range: " << inet_ntoa(netconfig.range_start) << " - " << inet_ntoa(netconfig.range_end) << std::endl;
    }
} // namespace tinydhcpd
