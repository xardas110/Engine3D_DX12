#pragma once

#include <iostream>
#include <sstream>
#include <cstdarg>

namespace Logger
{
	enum class LogLevel {
		_DEFAULT_,
		_DEBUG_,
		_INFO_,
		_WARNING_,
		_ERROR_
	};

	template <LogLevel level>
	void log(const char* format, ...) {
		std::ostringstream oss;

		// Set color based on log level
		const char* color = "";
		switch (level) {
		case LogLevel::_DEBUG_:
			color = ""; // Yellow
			break;
		case LogLevel::_INFO_:
			color = ""; // Green
			break;
		case LogLevel::_WARNING_:
			color = ""; // Yellow
			break;
		case LogLevel::_ERROR_:
			color = ""; // Red
			break;
		}

		oss << levelToStr(level);

		va_list args;
		va_start(args, format);
		char buffer[256];
		vsnprintf(buffer, sizeof(buffer), format, args);
		va_end(args);

		oss << color << buffer << "" << std::endl; // Reset color after log message
		std::cout << oss.str();
	}

	constexpr const char* levelToStr(LogLevel level) {
		switch (level) {
		case LogLevel::_DEBUG_:
			return "[DEBUG] ";
		case LogLevel::_INFO_:
			return "[INFO] ";
		case LogLevel::_WARNING_:
			return "[WARNING] ";
		case LogLevel::_ERROR_:
			return "[ERROR] ";
		case LogLevel::_DEFAULT_:
			return "";
		}
		return "UNKNOWN";
	}
}

#ifdef DEBUG_MODE
#define LOG_DEBUG(...) Logger::log<Logger::LogLevel::_DEBUG_>(__VA_ARGS__)
#else
#define LOG_DEBUG(...) do {} while (0)
#endif

#define LOG(...) Logger::log<Logger::LogLevel::_DEFAULT_>(__VA_ARGS__)
#define LOG_INFO(...) Logger::log<Logger::LogLevel::_INFO_>(__VA_ARGS__)
#define LOG_WARNING(...) Logger::log<Logger::LogLevel::_WARNING_>(__VA_ARGS__)
#define LOG_ERROR(...) Logger::log<Logger::LogLevel::_ERROR_>(__VA_ARGS__)
