#pragma once

// adapted from
// https://stackoverflow.com/questions/2638654/redirect-c-stdclog-to-syslog-on-unix

#include <ostream>
#include <streambuf>
#include <syslog.h>

namespace tinydhcpd {
enum SyslogPriority {
  SYSLOG_EMERG = LOG_EMERG,   // system is unusable
  SYSLOG_ALERT = LOG_ALERT,   // action must be taken immediately
  SYSLOG_CRIT = LOG_CRIT,     // critical conditions
  SYSLOG_ERR = LOG_ERR,       // error conditions
  SYSLOG_WARN = LOG_WARNING,  // warning conditions
  SYSLOG_NOTICE = LOG_NOTICE, // normal, but significant, condition
  SYSLOG_INFO = LOG_INFO,     // informational message
  SYSLOG_DEBUG = LOG_DEBUG    // debug-level message
};

std::ostream &operator<<(std::ostream &os, const SyslogPriority &log_priority);

class SyslogBuffer : public std::basic_streambuf<char, std::char_traits<char>> {
public:
  explicit SyslogBuffer(const std::string ident, const int facility);

protected:
  int sync();
  int overflow(int c);

private:
  friend std::ostream &operator<<(std::ostream &os,
                                  const SyslogPriority &log_priority);
  std::string buffer;
  int facility;
  int next_prio;
  std::string ident;
};

std::ostream syslog_stream(new SyslogBuffer("tinydhcpd", LOG_DAEMON));
} // namespace tinydhcpd
