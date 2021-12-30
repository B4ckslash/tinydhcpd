#include <arpa/inet.h>

#include <iostream>
#include <csignal>

#include "daemon.hpp"
#include "socket.hpp"

void sighandler(int signum)
{
    std::cout << "Caught signal " << signum << std::endl;
    tinydhcpd::last_signal = signum;
}

int main()
{
    std::signal(SIGINT, sighandler);
    std::signal(SIGTERM, sighandler);
    std::signal(SIGHUP, sighandler);

    struct in_addr addr;
    inet_aton("0.0.0.0", &addr);
    tinydhcpd::Daemon daemon(addr, "lo");
    std::cout << "finished" << std::endl;
    return 0;
}