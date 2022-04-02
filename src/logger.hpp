#include <iostream>
#define LOG(Logger_, Message_)                                                 \
  Logger_(static_cast<std::ostringstream &>(std::ostringstream().flush()       \
                                            << Message_)                       \
              .str())

namespace tinydhcpd {
enum Level { TRACE = 0, DEBUG, INFO, WARN, ERROR, FATAL };

class LogSink {
private:
  std::ostream &sink;
  virtual std::string format_message(const std::string &msg,
                                     Level level) const = 0;

public:
  LogSink(std::ostream &sink);
  void write(const std::string &msg, Level level) const;
  virtual ~LogSink() {}
};

class Logger {
private:
  Level lvl;
  const LogSink &sink;

public:
  Logger(const LogSink &sink, const Level level = INFO);
  void operator()(const std::string &message);
};
} // namespace tinydhcpd
