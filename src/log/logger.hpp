#pragma once

#include "logsink.hpp"
#include <memory>
#include <sstream>

namespace tinydhcpd {

extern Level current_global_log_level;

class Logger {
private:
  const LogSink &sink;

public:
  Logger(const LogSink &sink);
  void operator()(const std::string &message, const Level level);
};

extern std::unique_ptr<Logger> global_logger;

void LOG(Logger &Logger, const std::string &message, Level &level);
void LOG_TRACE(const std::string &msg);
void LOG_DEBUG(const std::string &msg);
void LOG_INFO(const std::string &msg);
void LOG_WARN(const std::string &msg);
void LOG_ERROR(const std::string &msg);
void LOG_FATAL(const std::string &msg);
} // namespace tinydhcpd
