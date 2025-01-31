#include "ProblemGenerationLogger.h"

#include "Clock.h"

namespace ProblemGenerationLog {
ProblemGenerationFileLogger::ProblemGenerationFileLogger(
    const std::filesystem::path& logFilePath)
    : logFilePath_(logFilePath) {
  SetType(LogUtils::LOGGERTYPE::FILE);
  logFile_.open(logFilePath_, std::ofstream::out | std::ofstream::app);
  if (logFile_.fail()) {
    std::cerr << "ProblemGenerationFileLogger: Invalid file ("
              << logFilePath_.string() << ") passed as parameter" << std::endl;
  }
}
void ProblemGenerationFileLogger::DisplayMessage(const std::string& message) {
  logFile_ << message << std::endl;
  logFile_.flush();
}

ProblemGenerationOstreamLogger::ProblemGenerationOstreamLogger(
    std::ostream& stream)
    : stream_(stream) {
  SetType(LogUtils::LOGGERTYPE::CONSOLE);
  if (stream_.fail()) {
    std::cerr
        << "ProblemGenerationOstreamLogger: Invalid stream passed as parameter"
        << std::endl;
  }
}
void ProblemGenerationOstreamLogger::DisplayMessage(
    const std::string& message) {
  stream_ << message << std::endl;
}
void ProblemGenerationLogger::AddLogger(
    const ProblemGenerationILoggerSharedPointer& logger) {
  loggers_.push_back(logger);
  try_to_add_logger_to_enabled_list(logger);
}
void ProblemGenerationLogger::DisplayMessage(const std::string& message) const {
  for (const auto& logger : enabled_loggers_) {
    logger->DisplayMessage(message);
  }
}
void ProblemGenerationLogger::DisplayMessage(
    const std::string& message, const LogUtils::LOGLEVEL log_level) const {
  for (const auto& logger : enabled_loggers_) {
    logger->DisplayMessage(LogUtils::LogLevelToStr(log_level));
    logger->DisplayMessage(message);
  }
}
void ProblemGenerationLogger::setLogLevel(const LogUtils::LOGLEVEL log_level) {
  log_level_ = log_level;
  update_enabled_logger();
}
void ProblemGenerationLogger::update_enabled_logger() {
  enabled_loggers_.clear();
  for (const auto& logger : loggers_) {
    try_to_add_logger_to_enabled_list(logger);
  }
}
bool ProblemGenerationLogger::try_to_add_logger_to_enabled_list(
    const ProblemGenerationILoggerSharedPointer& logger) {
  if ((logger->Type() == LogUtils::LOGGERTYPE::CONSOLE &&
       log_level_ != LogUtils::LOGLEVEL::DEBUG &&
       log_level_ != LogUtils::LOGLEVEL::TRACE) ||
      logger->Type() == LogUtils::LOGGERTYPE::FILE) {
    enabled_loggers_.insert(logger);
    return true;
  }
  return false;
}

ProblemGenerationLogger& ProblemGenerationLogger::operator<<(
    const LogUtils::LOGLEVEL log_level) {
  setLogLevel(log_level);
  for (const auto& subLogger : enabled_loggers_) {
    subLogger->GetOstreamObject() << PrefixMessage(log_level, context_);
  }
  return *this;
}

ProblemGenerationLogger& ProblemGenerationLogger::operator<<(
    std::ostream& (*f)(std::ostream&)) {
  for (const auto& subLogger : enabled_loggers_) {
    subLogger->GetOstreamObject() << f;
  }
  return *this;
}

ProblemGenerationLoggerSharedPointer BuildLogger(
    const std::filesystem::path& log_file_path, std::ostream& stream,
    const std::string& context) {
  auto logFile = std::make_shared<ProblemGenerationFileLogger>(log_file_path);
  auto logStd = std::make_shared<ProblemGenerationOstreamLogger>(std::cout);

  auto logger =
      std::make_shared<ProblemGenerationLogger>(LogUtils::LOGLEVEL::INFO);
  logger->AddLogger(logFile);
  logger->AddLogger(logStd);
  logger->setContext(context);
  return logger;
}
}  // namespace ProblemGenerationLog