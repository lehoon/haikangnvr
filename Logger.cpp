#include "Logger.h"
#include "common_define.h"

#include <spdlog/sinks/daily_file_sink.h>

static onceToken token([]() {
	auto logger = spdlog::daily_logger_mt("daily_logger", "logs/haikangnvr.log", 0, 0, false, 180);
	spdlog::set_pattern("[%Y-%m-%d %T] [thread %t] [%@ %!] [%l] %v");
	spdlog::set_default_logger(logger);
	spdlog::set_level(spdlog::level::debug);
	spdlog::flush_on(spdlog::level::info);
	spdlog::flush_every(std::chrono::seconds(1));

	}, []() {
		spdlog::drop_all();
		spdlog::shutdown();
	});


static bool SetConsoleColor(WORD Color)
{
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	if (handle == 0)
		return false;

	BOOL ret = SetConsoleTextAttribute(handle, Color);
	return(ret == TRUE);
}

bool Logger::SetConsoleColorInfo() {
	return SetConsoleColor(FOREGROUND_GREEN);
}

bool Logger::SetConsoleColorError() {
	return SetConsoleColor(FOREGROUND_RED);
}

bool Logger::SetConsoleColorDebug() {
	return SetConsoleColor(FOREGROUND_BLUE);
}

bool Logger::SetConsoleColorYellow() {
	return SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN);
}

bool Logger::RestoreConsoleColor() {
	return SetConsoleColor(0x0007);
}

