#pragma once
#include <string>

class Helper
{
public:
	static std::string decode_name(bool decode);
	static std::string translate_mat(bool mat);
	static std::string logger_open(bool logger);
	static std::string login_message(int code);
};

