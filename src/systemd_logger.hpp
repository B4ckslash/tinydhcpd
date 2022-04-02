#include "logger.hpp"
#include <systemd/sd-daemon.h>

#ifndef LOG_TRACE
#define LOG_TRACE(Message_) LOG(tinydhcpd::TraceLogger(), Message_)
#endif
#ifndef LOG_DEBUG
#define LOG_DEBUG(Message_) LOG(tinydhcpd::DebugLogger(), Message_)
#endif
#ifndef LOG_INFO
#define LOG_INFO(Message_) LOG(tinydhcpd::InfoLogger(), Message_)
#endif
#ifndef LOG_WARN
#define LOG_WARN(Message_) LOG(tinydhcpd::WarningLogger(), Message_)
#endif
#ifndef LOG_ERROR
#define LOG_ERROR(Message_) LOG(tinydhcpd::ErrorLogger(), Message_)
#endif
#ifndef LOG_FATAL
#define LOG_FATAL(Message_) LOG(tinydhcpd::FatalLogger(), Message_)
#endif

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

static Logger &TraceLogger() {
  static Logger logger(SystemdLogSink(std::cerr), Level::TRACE);
  return logger;
}
static Logger &DebugLogger() {
  static Logger logger(SystemdLogSink(std::cerr), Level::DEBUG);
  return logger;
}
static Logger &InfoLogger() {
  static Logger logger(SystemdLogSink(std::cerr), Level::INFO);
  return logger;
}
static Logger &WarningLogger() {
  static Logger logger(SystemdLogSink(std::cerr), Level::WARN);
  return logger;
}
static Logger &ErrorLogger() {
  static Logger logger(SystemdLogSink(std::cerr), Level::ERROR);
  return logger;
}
static Logger &FatalLogger() {
  static Logger logger(SystemdLogSink(std::cerr), Level::FATAL);
  return logger;
}
} // namespace tinydhcpd
