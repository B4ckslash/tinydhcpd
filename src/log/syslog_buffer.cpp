#include "syslog_buffer.hpp"

namespace tinydhcpd {
SyslogBuffer::SyslogBuffer(const std::string ident, const int facility)
    : facility(facility), next_prio(SYSLOG_DEBUG), ident(ident) {
  openlog(ident.c_str(), LOG_PID, facility);
}

int SyslogBuffer::sync() {
  if (buffer.length()) {
    syslog(next_prio, "%s", buffer.c_str());
    buffer.erase();
    next_prio = SYSLOG_DEBUG;
  }
  return 0;
}

int SyslogBuffer::overflow(int c) {
  if (c != std::char_traits<char>::eof()) {
    buffer += static_cast<char>(c);
  } else {
    sync();
  }
  return c;
}

std::ostream &operator<<(std::ostream &os, const SyslogPriority &prio) {
  static_cast<SyslogBuffer *>(os.rdbuf())->next_prio = static_cast<int>(prio);
  return os;
}

std::ostream syslog_stream(new SyslogBuffer("tinydhcpd", LOG_DAEMON));
} // namespace tinydhcpd
