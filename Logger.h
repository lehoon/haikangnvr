#ifndef HAIKANG_NVR_TESTION_LOGGER_H_
#define HAIKANG_NVR_TESTION_LOGGER_H_


#include <windows.h>

#include <spdlog/spdlog.h>

class Logger
{
public:
	Logger() = delete;
	~Logger() = delete;

public:

	static bool SetConsoleColorInfo();
	static bool SetConsoleColorError();
	static bool SetConsoleColorDebug();
	static bool SetConsoleColorYellow();
	static bool RestoreConsoleColor();
};

#define CONSOLE_COLOR_INFO      Logger::SetConsoleColorInfo
#define CONSOLE_COLOR_ERROR     Logger::SetConsoleColorError
#define CONSOLE_COLOR_DEBUG     Logger::SetConsoleColorDebug
#define CONSOLE_COLOR_YELLOW    Logger::SetConsoleColorYellow
#define CONSOLE_COLOR_RESET     Logger::RestoreConsoleColor


#endif //HAIKANG_NVR_TESTION_LOGGER_H_
