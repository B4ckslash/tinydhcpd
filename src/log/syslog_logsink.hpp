#pragma once

#include <chrono>
#include <ctime>
#include <sstream>

#include "logsink.hpp"
#include "src/string-format.hpp"
#include "syslog_buffer.hpp"

namespace tinydhcpd {
class SyslogLogsink : public LogSink {
private:
  virtual std::string format_message(const std::string &msg,
                                     Level level) const override {
    switch (level) {
    case TRACE:
      sink << SYSLOG_DEBUG;
      break;
    case DEBUG:
      sink << SYSLOG_DEBUG;
      break;
    case WARN:
      sink << SYSLOG_WARN;
      break;
    case ERROR:
      sink << SYSLOG_ALERT;
      break;
    case FATAL:
      sink << SYSLOG_CRIT;
      break;
    case INFO:
    default:
      sink << SYSLOG_INFO;
      break;
    }

    return msg;
  }

public:
  SyslogLogsink() : LogSink(tinydhcpd::syslog_stream) {}
  ~SyslogLogsink() {}
};
} // namespace tinydhcpd
