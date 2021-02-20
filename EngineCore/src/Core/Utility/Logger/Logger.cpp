#include "Logger.h"

fne::LogLevel fne::Logger::m_loglevel = fne::LogLevel::Error;
const std::string fne::Logger::m_loglevelTexts[] = { "DEBUG", " INFO", " WARN", "ERROR", "FATAL" };
