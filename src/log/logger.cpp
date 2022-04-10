#include "logger.hpp"

namespace tinydhcpd {
Level current_global_log_level = Level::TRACE;

Logger::Logger(const LogSink &sink) : sink(sink) {}

void Logger::operator()(const std::string &message, const Level lvl) {
  if (static_cast<int>(lvl) < current_global_log_level)
    return;
  sink.write(message, lvl);
}
} // namespace tinydhcpd
