#ifndef DAEMON_HPP
#define DAEMON_HPP
#include <iostream>
#include <libconfig.h++>

#include "socket.hpp"

namespace tinydhcpd
{
    class Daemon 
    {
        private:
            Socket socket;

        public:
            Daemon(uint8_t listen_address[4]);
            Daemon(const std::string& iface_name);
    };

} // namespace tinydhcpd
#endif