#include <iostream>
#include <getopt.h>
#include "daemon.hpp"

int main(int argc, char *argv[])
{
    uint8_t addr[] = {10,1,1,0};
    tinydhcpd::Daemon daemon(addr);
    tinydhcpd::Daemon daemon_iface(std::string("eth0"));
    std::cout << "finished" << std::endl;
    return 0;
}