#include "common/logger.h"

#include <iostream>

namespace fnfinder {

namespace {

const char* levelTag(LogLevel level) {
  switch (level) {
    case LogLevel::kTrace: return "[TRACE]";
    case LogLevel::kDebug: return "[DEBUG]";
    case LogLevel::kInfo:  return "[INFO ]";
    case LogLevel::kWarn:  return "[WARN ]";
    case LogLevel::kError: return "[ERROR]";
    case LogLevel::kOff:   return "[OFF  ]";
  }
  return "[?????]";
}

}

Logger& Logger::instance() {
  static Logger logger;
  return logger;
}

void Logger::log(LogLevel level, const std::string& message) {
  if (!isEnabled(level)) {
    return;
  }
  std::cerr << levelTag(level) << ' ' << message << '\n';
}

void logTrace(const std::string& message) { Logger::instance().log(LogLevel::kTrace, message); }
void logDebug(const std::string& message) { Logger::instance().log(LogLevel::kDebug, message); }
void logInfo(const std::string& message)  { Logger::instance().log(LogLevel::kInfo, message); }
void logWarn(const std::string& message)  { Logger::instance().log(LogLevel::kWarn, message); }
void logError(const std::string& message) { Logger::instance().log(LogLevel::kError, message); }

}
