#pragma once

#include "logsink.hpp"
#include <memory>

#define LOG(Logger_, Message_)                                                 \
  Logger_(static_cast<std::ostringstream &>(std::ostringstream().flush()       \
                                            << Message_)                       \
              .str())
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

extern std::unique_ptr<LogSink> global_sink;

class Logger {
private:
  Level lvl;
  const LogSink &sink;

public:
  Logger(const LogSink &sink, const Level level = INFO);
  void operator()(const std::string &message);
};

static Logger &TraceLogger() {
  static Logger logger(*global_sink, Level::TRACE);
  return logger;
}
static Logger &DebugLogger() {
  static Logger logger(*global_sink, Level::DEBUG);
  return logger;
}
static Logger &InfoLogger() {
  static Logger logger(*global_sink, Level::INFO);
  return logger;
}
static Logger &WarningLogger() {
  static Logger logger(*global_sink, Level::WARN);
  return logger;
}
static Logger &ErrorLogger() {
  static Logger logger(*global_sink, Level::ERROR);
  return logger;
}
static Logger &FatalLogger() {
  static Logger logger(*global_sink, Level::FATAL);
  return logger;
}
} // namespace tinydhcpd
