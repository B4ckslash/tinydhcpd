#include "logger.hpp"

namespace tinydhcpd {
Level current_global_log_level = Level::INFO;

Logger::Logger(const LogSink &sink) : sink(sink) {}

void Logger::operator()(const std::string &message, const Level lvl) {
  if (static_cast<int>(lvl) < current_global_log_level)
    return;
  sink.write(message, lvl);
}

void LOG(const std::string &message, const Level &level) {
  (*tinydhcpd::global_logger)(message, level);
}

void LOG_TRACE([[maybe_unused]] const std::string &msg) {
#ifdef ENABLE_TRACE
  LOG(msg, Level::TRACE);
#else
  return;
#endif
}

void LOG_DEBUG(const std::string &msg) { LOG(msg, Level::DEBUG); }
void LOG_INFO(const std::string &msg) { LOG(msg, Level::INFO); }
void LOG_WARN(const std::string &msg) { LOG(msg, Level::WARN); }
void LOG_ERROR(const std::string &msg) { LOG(msg, Level::ERROR); }
void LOG_FATAL(const std::string &msg) { LOG(msg, Level::FATAL); }
} // namespace tinydhcpd
