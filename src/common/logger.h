#ifndef FNFINDER_COMMON_LOGGER_H_
#define FNFINDER_COMMON_LOGGER_H_

#include <string>

namespace fnfinder {

enum class LogLevel {
  kTrace = 0,
  kDebug,
  kInfo,
  kWarn,
  kError,
  kOff,
};

class Logger {
 public:
  static Logger& instance();

  void setLevel(LogLevel level) { level_ = level; }
  LogLevel level() const { return level_; }
  bool isEnabled(LogLevel level) const { return level >= level_; }

  void log(LogLevel level, const std::string& message);

 private:
  Logger() = default;

  LogLevel level_ = LogLevel::kInfo;
};

void logTrace(const std::string& message);
void logDebug(const std::string& message);
void logInfo(const std::string& message);
void logWarn(const std::string& message);
void logError(const std::string& message);

}

#endif
