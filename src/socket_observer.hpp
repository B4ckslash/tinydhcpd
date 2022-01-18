#pragma once

#include "datagram.hpp"

namespace tinydhcpd {

class SocketObserver {
public:
  virtual ~SocketObserver(){};

  virtual void handle_recv(tinydhcpd::DhcpDatagram &datagram) = 0;
};
} // namespace tinydhcpd
