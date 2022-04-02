#include "logger.hpp"

namespace tinydhcpd {
Logger::Logger(const LogSink &sink, const Level level)
    : lvl(level), sink(sink) {}

void Logger::operator()(const std::string &message) {
  sink.write(message, lvl);
}

LogSink::LogSink(std::ostream &sink) : sink(sink) {}

void LogSink::write(const std::string &msg, Level level) const {
  sink << format_message(msg, level) << std::endl;
}
} // namespace tinydhcpd
