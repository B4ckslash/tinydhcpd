#pragma once
#include <iostream>

namespace tinydhcpd {
enum Level { TRACE = 0, DEBUG, INFO, WARN, ERROR, FATAL };

class LogSink {
protected:
  std::ostream &sink;
  virtual std::string format_message(const std::string &msg,
                                     Level level) const = 0;

public:
  LogSink(std::ostream &sink) : sink(sink) {}

  void write(const std::string &msg, Level level) const {
    const std::string formatted = format_message(msg, level);
    sink << formatted << std::endl;
  }
  virtual ~LogSink() {}
};
} // namespace tinydhcpd
