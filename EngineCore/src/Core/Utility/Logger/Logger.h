#pragma once
#include <string>
#include <fmt/core.h>

namespace fne {

struct LogReportData {
	std::string fileName;
	std::string funcName;
	int line;
};


enum class LogLevel { Debug = 0, Info, Warn, Error, Fatal };

class Logger {
public:
	static inline void SetLogLevel(LogLevel newLevel) { m_loglevel = newLevel; }
	
	static void Log(const std::string& category, const LogReportData& report, const LogLevel level, const std::string& message) {
#ifndef _DEBUG
		return;
#endif
		if (level < m_loglevel) {
			return;
		}
		if (level < LogLevel::Debug || level > LogLevel::Fatal) {
			return;
		}

		auto levelText = m_loglevelTexts[static_cast<int>(level)];

		// 1. [WARN] [parent/category1] "ログメッセージ" (__FILE__ __LINE_ __func__)
		OutputDebugStringA(fmt::format("[{0}] [{1}] \"{2}\" ({3} {4} {5})\n",
			levelText ,category, message,
			report.fileName, report.line, report.funcName).c_str());
		return;
	}
private:
	static LogLevel m_loglevel;
	static const std::string m_loglevelTexts[];
};

}

fne::LogLevel fne::Logger::m_loglevel = fne::LogLevel::Error;
const std::string fne::Logger::m_loglevelTexts[] = { "DEBUG", " INFO", " WARN", "ERROR", "FATAL" };

#define CURRENT_REPORT_DATA fne::LogReportData{__FILE__,__func__,__LINE__}
#define LOG_DEBUG(category, log) fne::Logger::Log(category,CURRENT_REPORT_DATA,fne::LogLevel::Debug,log)
#define LOG_INFO(category, log)  fne::Logger::Log(category,CURRENT_REPORT_DATA,fne::LogLevel::Info,log)
#define LOG_WARN(category, log)  fne::Logger::Log(category,CURRENT_REPORT_DATA,fne::LogLevel::Warn,log)
#define LOG_ERROR(category, log) fne::Logger::Log(category,CURRENT_REPORT_DATA,fne::LogLevel::Error,log)
#define LOG_FATAL(category, log) fne::Logger::Log(category,CURRENT_REPORT_DATA,fne::LogLevel::Fatal,log)

