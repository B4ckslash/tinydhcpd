#pragma once

#include "logsink.hpp"
#include <memory>

#define LOG(Logger_, Message_, Level_)                                         \
  Logger_(static_cast<std::ostringstream &>(std::ostringstream().flush()       \
                                            << Message_)                       \
              .str(),                                                          \
          Level_)
#ifndef LOG_TRACE
#define LOG_TRACE(Message_)                                                    \
  LOG(tinydhcpd::LogInstance(), Message_, tinydhcpd::Level::TRACE)
#endif
#ifndef LOG_DEBUG
#define LOG_DEBUG(Message_)                                                    \
  LOG(tinydhcpd::LogInstance(), Message_, tinydhcpd::Level::DEBUG)
#endif
#ifndef LOG_INFO
#define LOG_INFO(Message_)                                                     \
  LOG(tinydhcpd::LogInstance(), Message_, tinydhcpd::Level::INFO)
#endif
#ifndef LOG_WARN
#define LOG_WARN(Message_)                                                     \
  LOG(tinydhcpd::LogInstance(), Message_, tinydhcpd::Level::WARN)
#endif
#ifndef LOG_ERROR
#define LOG_ERROR(Message_)                                                    \
  LOG(tinydhcpd::LogInstance(), Message_, tinydhcpd::Level::ERROR)
#endif
#ifndef LOG_FATAL
#define LOG_FATAL(Message_)                                                    \
  LOG(tinydhcpd::LogInstance(), Message_, tinydhcpd::Level::INFO)
#endif

namespace tinydhcpd {

extern std::unique_ptr<LogSink> global_sink;

class Logger {
private:
  const LogSink &sink;

public:
  Logger(const LogSink &sink);
  void operator()(const std::string &message, const Level level);
};

static Logger &LogInstance() {
  static Logger logger(*global_sink);
  return logger;
};
} // namespace tinydhcpd
