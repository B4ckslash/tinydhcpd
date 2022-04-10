#pragma once

#include "logsink.hpp"
#include <memory>
#include <sstream>

#define LOG(Logger_, Message_, Level_)                                         \
  Logger_(static_cast<std::ostringstream &>(std::ostringstream().flush()       \
                                            << Message_)                       \
              .str(),                                                          \
          Level_)
#ifdef ENABLE_TRACE
#define LOG_TRACE(Message_)                                                    \
  LOG(tinydhcpd::LogInstance(), Message_, tinydhcpd::Level::TRACE)
#else
#define LOG_TRACE(Message_)                                                    \
  do {                                                                         \
  } while (0)
#endif

#define LOG_DEBUG(Message_)                                                    \
  LOG(tinydhcpd::LogInstance(), Message_, tinydhcpd::Level::DEBUG)

#define LOG_INFO(Message_)                                                     \
  LOG(tinydhcpd::LogInstance(), Message_, tinydhcpd::Level::INFO)

#define LOG_WARN(Message_)                                                     \
  LOG(tinydhcpd::LogInstance(), Message_, tinydhcpd::Level::WARN)

#define LOG_ERROR(Message_)                                                    \
  LOG(tinydhcpd::LogInstance(), Message_, tinydhcpd::Level::ERROR)

#define LOG_FATAL(Message_)                                                    \
  LOG(tinydhcpd::LogInstance(), Message_, tinydhcpd::Level::FATAL)

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
