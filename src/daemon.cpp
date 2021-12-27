#include "daemon.hpp"


namespace tinydhcpd
{
    Daemon::Daemon(const struct in_addr& listen_address)
        try : socket{ listen_address, *this }
    {
        socket.recv_loop();
    }
    catch (std::runtime_error& ex)
    {
        printf("%s\n", ex.what());
        exit(EXIT_FAILURE);
    }

    Daemon::Daemon(const std::string& iface_name)
        try : socket{ iface_name, *this }
    {
        std::cout << "bind to iface: " << iface_name << std::endl;
    }
    catch (std::runtime_error& ex)
    {
        printf("%s\n", ex.what());
        exit(EXIT_FAILURE);
    }

    void Daemon::handle_recv(DhcpDatagram& datagram)
    {
        printf("handle_recv\n");
        printf("%d\n", datagram.hw_addr[0]);
    }
} // namespace tinydhcpd
