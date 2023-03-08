#include "Helper.h"


std::string Helper::decode_name(bool decode) {
	return decode ? "是" : "否";
}

std::string Helper::translate_mat(bool mat) {
	return mat ? "生成" : "不生成";
}

std::string Helper::logger_open(bool logger) {
	return logger ? "启用" : "禁用";
}

std::string Helper::login_message(int code) {
	return code > 0 ? "成功" : "失败";
}