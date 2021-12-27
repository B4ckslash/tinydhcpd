#include <iostream>
#include <getopt.h>
#include "daemon.hpp"

int main(int argc, char *argv[])
{
    uint32_t addr = (uint8_t)10 << 24 + (uint8_t)1 << 16 + uint8_t(200) << 8 + 1;
    tinydhcpd::Daemon daemon(addr);
    tinydhcpd::Daemon daemon_iface(std::string("eth0"));
    std::cout << "finished" << std::endl;
    return 0;
}