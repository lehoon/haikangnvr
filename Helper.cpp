#include "Helper.h"


std::string Helper::decode_name(bool decode) {
	return decode ? "��" : "��";
}

std::string Helper::translate_mat(bool mat) {
	return mat ? "����" : "������";
}

std::string Helper::logger_open(bool logger) {
	return logger ? "����" : "����";
}

std::string Helper::login_message(int code) {
	return code > 0 ? "�ɹ�" : "ʧ��";
}