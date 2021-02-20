#include "Logger.h"
#include <fmt/format.h>
#include <fmt/core.h>

fne::LogLevel fne::Logger::m_loglevel = fne::LogLevel::Error;
const std::string fne::Logger::m_loglevelTexts[] = { "DEBUG", " INFO", " WARN", "ERROR", "FATAL" };
const std::wstring fne::Logger::m_loglevelTextsW[] = { L"DEBUG", L" INFO", L" WARN", L"ERROR", L"FATAL" };

void fne::Logger::Log(const std::string& category, const LogReportData& report, const LogLevel level, const std::string& message) {
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
		levelText, category, message,
		report.fileName, report.line, report.funcName).c_str());
}

void fne::Logger::Log(const std::wstring& category, const LogReportDataW& report, const LogLevel level, const std::wstring& message)
{
#ifndef _DEBUG
	return;
#endif
	if (level < m_loglevel) {
		return;
	}
	if (level < LogLevel::Debug || level > LogLevel::Fatal) {
		return;
	}

	auto levelText = m_loglevelTextsW[static_cast<int>(level)];

	// __func__(char*)をwchar_t*に変換
	assert(report.funcName.length() < 50);
	wchar_t wcbuf[50];
	MultiByteToWideChar(CP_ACP, 0, report.funcName.c_str(), -1, wcbuf, (int)report.funcName.length());
	wcbuf[report.funcName.length()] = '\0';
	std::wstring wfuncname_str = wcbuf;

	// 1. [WARN] [parent/category1] "ログメッセージ" (__FILE__ __LINE_ __func__)
	OutputDebugStringW(fmt::format(L"[{0}] [{1}] \"{2}\" ({3} {4} {5})\n",
		levelText, category, message,
		report.fileName, report.line, wfuncname_str).c_str());
	return;
}
