#include <iostream>
#include <arpa/inet.h>
#include <getopt.h>
#include "daemon.hpp"

int main(int argc, char *argv[])
{
    struct in_addr addr;
    inet_aton("10.1.200.1", &addr); 
    std::cout << addr.s_addr << std::endl;
    tinydhcpd::Daemon daemon(addr);
    tinydhcpd::Daemon daemon_iface(std::string("eth0"));
    std::cout << "finished" << std::endl;
    return 0;
}