#include "logger.hpp"

namespace tinydhcpd {
Logger::Logger(const LogSink &sink, const Level level)
    : lvl(level), sink(sink) {}

void Logger::operator()(const std::string &message) {
  if (static_cast<int>(lvl) < current_log_level)
    return;
  sink.write(message, lvl);
}
} // namespace tinydhcpd
