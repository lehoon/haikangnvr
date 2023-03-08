#include "Configure.h"
#include "common_define.h"
#include <stdlib.h>
#include <iostream>


class ConfigFile {
public:
	ConfigFile();
	~ConfigFile();

	int Init(const char* file);

	int ParseFile(std::map<std::string, std::map<std::string, std::string> >& group_items);
private:
	int GetLine(std::string& line);
	int GetKeyAndValue(const std::string& line, std::string& key, std::string& value);

	void Trim(std::string& str);
private:
	FILE* pf_;
	enum {
		kErrorRow = 0,
		kEmptyRow,
		kGroupRow,
		kValueRow,
	};

};

ConfigFile::ConfigFile() : pf_(NULL) {}

ConfigFile::~ConfigFile() {
	if (pf_ != NULL) {
		fclose(pf_);
	}
}

int ConfigFile::Init(const char* file) {
	pf_ = fopen(file, "r");
	if (pf_ == NULL) return RESULT_ERROR;
	return RESULT_OK;
}

void ConfigFile::Trim(std::string& str) {
	size_t size = str.size();
	if (size == 0) return;
	size_t begin = 0;
	while (begin < size && str[begin] == ' ') begin++;
	size_t end = size - 1;
	while (end > begin && str[end] == ' ') end--;
	if (end >= begin) {
		str = str.substr(begin, end + 1 - begin);
	}
	else {
		str = "";
	}
}

int ConfigFile::GetLine(std::string& line) {
	const int kLen = 512;
	char buf[kLen] = { 0 };
	if (fgets(buf, kLen, pf_) != NULL) {
		line = buf;
		Trim(line);
		if (line.length() < 2) return kEmptyRow;
		if (line[0] == '#') return kEmptyRow;

		std::string s = line.substr(line.length() - 2);
		if (s == "\n\r") {
			line = line.substr(0, line.length() - 2);
		}
		s = line.substr(line.length() - 1);
		if (s == "\r" || s == "\n") {
			line = line.substr(0, line.length() - 1);
		}

		if (line.length() == 0) return kEmptyRow;
		if ((line[0] == '[') && (line[line.length() - 1] == ']')) {
			line = line.substr(1, line.length() - 2);
			return kGroupRow;
		}
		return kValueRow;
	}

	//包括
	return kErrorRow;
}

int ConfigFile::GetKeyAndValue(const std::string& line, std::string& key, std::string& value) {
	int pos = line.find_first_of("=");
	if (pos < 0) return RESULT_ERROR;

	key = line.substr(0, pos);
	Trim(key);

	value = line.substr(pos + 1);
	Trim(value);

	return RESULT_OK;
}

int ConfigFile::ParseFile(std::map<std::string, std::map<std::string, std::string> >& group_items) {
	fseek(pf_, 0, SEEK_SET);
	int type = kErrorRow;
	std::string line;
	std::string group;
	std::string key;
	std::string value;

	while ((type = GetLine(line)) != kErrorRow) {
		if (type == kGroupRow) {
			group = line;
			group_items[group] = std::map<std::string, std::string>();
		}
		else if (type == kValueRow) {
			//找到key and value
			if (GetKeyAndValue(line, key, value) == RESULT_OK) {
				group_items[group][key] = value;
			}
		}
	}

	return RESULT_OK;
}

CConfigure::CConfigure() {

}

CConfigure::~CConfigure() {

}

int CConfigure::Init(const std::string& file) {
	ConfigFile config;
	int result = config.Init(file.c_str());
	if (result != RESULT_OK) return result;
	//解析具体配置文件:
	result = config.ParseFile(items_);
	return result;
}

void CConfigure::Dump2LogFile() {
	for (auto iter = items_.begin(); iter != items_.end(); iter++) {

		if (iter->first.compare("manager") == 0) {
			continue;
		}

		SPDLOG_INFO("打印配置文件项section:[{}]", iter->first);
		auto kv = iter->second;


		for (auto iter2 = kv.begin(); iter2 != kv.end(); iter2++) {
			SPDLOG_INFO("打印配置参数:                  {}====>>{}", iter2->first, iter2->second);
		}
	}
}

std::string CConfigure::GetValueAsString(const std::string& group,
	const std::string& key,
	const std::string& default_value) const {
	std::string result = default_value;
	std::map<std::string, std::map<std::string, std::string> >::const_iterator g_it = items_.find(group);
	if (g_it != items_.end()) {
		std::map<std::string, std::string>::const_iterator it = g_it->second.find(key);
		if (it != g_it->second.end()) {
			result = it->second;
		}
	}

	return result;
}

uint32_t CConfigure::GetValueAsUint32(const std::string& group,
	const std::string& key,
	uint32_t default_value) const {
	uint32_t result = default_value;
	std::map<std::string, std::map<std::string, std::string> >::const_iterator g_it = items_.find(group);
	if (g_it != items_.end()) {
		std::map<std::string, std::string>::const_iterator it = g_it->second.find(key);
		if (it != g_it->second.end()) {
			result = atoi(it->second.c_str());
		}
	}

	return result;
}

double CConfigure::GetValueAsDouble(const std::string& group,
	const std::string& key,
	double default_value) const {
	double result = default_value;
	std::map<std::string, std::map<std::string, std::string> >::const_iterator g_it = items_.find(group);
	if (g_it != items_.end()) {
		std::map<std::string, std::string>::const_iterator it = g_it->second.find(key);
		if (it != g_it->second.end()) {
			result = atof(it->second.c_str());
		}
	}

	return result;
}

bool CConfigure::GetValueAsBool(const std::string& group,
	const std::string& key,
	bool  default_value) const {
	bool result = default_value;
	std::map<std::string, std::map<std::string, std::string> >::const_iterator g_it = items_.find(group);
	if (g_it != items_.end()) {
		std::map<std::string, std::string>::const_iterator it = g_it->second.find(key);
		if (it != g_it->second.end()) {
			result = (it->second == "1");
		}
	}

	return result;
}

