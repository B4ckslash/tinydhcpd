#pragma once

#include <chrono>
#include <ctime>
#include <sstream>

#include "logsink.hpp"
#include "src/string-format.hpp"

namespace tinydhcpd {
class StdoutLogSink : public LogSink {
private:
  const std::string FORMAT_RESET = "\033[0m";
  const std::string YELLOW = "\033[33m";
  const std::string RED = "\033[31m";
  const std::string MAGENTA = "\033[35m";
  const std::string CYAN = "\033[36m";
  const std::string BLACK_ON_RED = "\033[30;41m";

  virtual std::string format_message(const std::string &msg,
                                     Level level) const override {
    std::stringstream buffer;
    const std::string format = "%-6s";
    switch (level) {
    case TRACE:
      buffer << CYAN << string_format(format, "TRACE");
      break;
    case DEBUG:
      buffer << MAGENTA << string_format(format, "DEBUG");
      break;
    case WARN:
      buffer << YELLOW << string_format(format, "WARN");
      break;
    case ERROR:
      buffer << RED << string_format(format, "ERROR");
      break;
    case FATAL:
      buffer << BLACK_ON_RED << string_format(format, "FATAL");
      break;
    case INFO:
    default:
      buffer << string_format(format, "INFO");
      break;
    }

    using std::chrono::system_clock;
    auto now = system_clock::to_time_t(system_clock::now());
    std::string ts = std::ctime(&now);

    if (ts.ends_with("\n")) {
      ts.pop_back();
    }
    buffer << "[" << ts << "] " << msg << FORMAT_RESET;
    return buffer.str();
  }

public:
  StdoutLogSink() : LogSink(std::cout) {}
  ~StdoutLogSink() {}
};
} // namespace tinydhcpd
