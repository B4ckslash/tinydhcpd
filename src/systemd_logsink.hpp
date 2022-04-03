#include "logsink.hpp"
#include <systemd/sd-daemon.h>

namespace tinydhcpd {
class SystemdLogSink : public LogSink {
private:
  virtual std::string format_message(const std::string &msg,
                                     Level level) const override {
    switch (level) {
    case TRACE:
      return SD_DEBUG + msg;
    case DEBUG:
      return SD_DEBUG + msg;
    case WARN:
      return SD_WARNING + msg;
    case ERROR:
      return SD_ERR + msg;
    case FATAL:
      return SD_CRIT + msg;
    case INFO:
    default:
      return SD_INFO + msg;
    }
  }

public:
  SystemdLogSink(std::ostream &sink) : LogSink(sink) {}
  ~SystemdLogSink() {}
};
} // namespace tinydhcpd
