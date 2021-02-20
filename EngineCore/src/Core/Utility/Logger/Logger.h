#pragma once
#include <string>

#define CURRENT_REPORT_DATA fne::LogReportData{__FILE__,__func__,__LINE__}
#define LOG_DEBUG(category, log) fne::Logger::Log(category,CURRENT_REPORT_DATA,fne::LogLevel::Debug,log)
#define LOG_INFO(category, log)  fne::Logger::Log(category,CURRENT_REPORT_DATA,fne::LogLevel::Info,log)
#define LOG_WARN(category, log)  fne::Logger::Log(category,CURRENT_REPORT_DATA,fne::LogLevel::Warn,log)
#define LOG_ERROR(category, log) fne::Logger::Log(category,CURRENT_REPORT_DATA,fne::LogLevel::Error,log)
#define LOG_FATAL(category, log) fne::Logger::Log(category,CURRENT_REPORT_DATA,fne::LogLevel::Fatal,log)

#define CURRENT_REPORT_DATA_W fne::LogReportDataW{__FILEW__,__func__,__LINE__}
#define LOG_DEBUG_W(category, log) fne::Logger::Log(category,CURRENT_REPORT_DATA_W,fne::LogLevel::Debug,log)
#define LOG_INFO_W(category, log)  fne::Logger::Log(category,CURRENT_REPORT_DATA_W,fne::LogLevel::Info,log)
#define LOG_WARN_W(category, log)  fne::Logger::Log(category,CURRENT_REPORT_DATA_W,fne::LogLevel::Warn,log)
#define LOG_ERROR_W(category, log) fne::Logger::Log(category,CURRENT_REPORT_DATA_W,fne::LogLevel::Error,log)
#define LOG_FATAL_W(category, log) fne::Logger::Log(category,CURRENT_REPORT_DATA_W,fne::LogLevel::Fatal,log)

namespace fne {

struct LogReportData {
	std::string fileName;
	std::string funcName;
	int line;
};

struct LogReportDataW {
	std::wstring fileName;
	std::string funcName;	// __file__のワイド文字列版は存在しないため
	int line;
};

enum class LogLevel { Debug = 0, Info, Warn, Error, Fatal };

class Logger {
public:
	static inline void SetLogLevel(LogLevel newLevel) { m_loglevel = newLevel; }
	
	static void Log(const std::string& category, const LogReportData& report, const LogLevel level, const std::string& message);
	static void Log(const std::wstring& category, const LogReportDataW& report, const LogLevel level, const std::wstring& message);

private:

	static LogLevel m_loglevel;
	static const std::string m_loglevelTexts[];
	static const std::wstring m_loglevelTextsW[];
};

}

