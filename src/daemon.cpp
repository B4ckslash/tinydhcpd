#include "daemon.hpp"


namespace tinydhcpd
{
    volatile std::sig_atomic_t last_signal;

    Daemon::Daemon(const struct in_addr& address, const std::string& iface_name)
        try : socket{ address, iface_name, *this }
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
        printf("handle_recv\n");
        printf("%#010x\n", datagram.transaction_id);
    }
} // namespace tinydhcpd
