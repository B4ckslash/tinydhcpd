#include <arpa/inet.h>
#include <getopt.h>
#include <libconfig.h++>

#include <iostream>
#include <csignal>

#include "daemon.hpp"
#include "socket.hpp"

#define ADDRESS_OPTION_TAG 'a'
#define IFACE_OPTION_TAG 'i'

void sighandler(int signum)
{
    std::cout << "Caught signal " << signum << std::endl;
    tinydhcpd::last_signal = signum;
}

struct option_values {
    struct in_addr address;
    std::string interface;
};

const struct option long_options[] = {
    { "address", required_argument, nullptr, ADDRESS_OPTION_TAG},
    { "interface", required_argument, nullptr, IFACE_OPTION_TAG},
    { nullptr, 0, nullptr, 0}
};

int main(int argc, char* const argv[])
{
    std::signal(SIGINT, sighandler);
    std::signal(SIGTERM, sighandler);
    std::signal(SIGHUP, sighandler);

    struct option_values optval = {
        .address = {.s_addr = INADDR_ANY},
        .interface = ""
    };

    char opt;
    while ((opt = getopt_long(argc, argv, "a:i:", long_options, nullptr)) != -1)
    {
        switch (opt)
        {
        case ADDRESS_OPTION_TAG:
            std::cout << "Using address: " << optarg << std::endl;
            inet_aton(optarg, &(optval.address));
            break;

        case IFACE_OPTION_TAG:
            std::cout << "Using iface: " << optarg << std::endl;
            optval.interface = optarg;
            break;

        default:
            break;
        }
    }

    tinydhcpd::Daemon daemon(optval.address, optval.interface);
    std::cout << "finished" << std::endl;
    return 0;
}