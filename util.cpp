/*
 * Copyright (c) 2016 The ZLToolKit project authors. All Rights Reserved.
 *
 * This file is part of ZLToolKit(https://github.com/ZLMediaKit/ZLToolKit).
 *
 * Use of this source code is governed by MIT license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <algorithm>
#include <random>

#include "util.h"

#if defined(_WIN32)
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
extern "C" const IMAGE_DOS_HEADER __ImageBase;
#endif // defined(_WIN32)

using namespace std;

namespace toolkit {

	static constexpr char CCH[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

	string makeRandStr(int sz, bool printable) {
		string ret;
		ret.resize(sz);
		std::mt19937 rng(std::random_device{}());
		for (int i = 0; i < sz; ++i) {
			if (printable) {
				uint32_t x = rng() % (sizeof(CCH) - 1);
				ret[i] = CCH[x];
			}
			else {
				ret[i] = rng() % 0xFF;
			}
		}
		return ret;
	}

	bool is_safe(uint8_t b) {
		return b >= ' ' && b < 128;
	}

	string hexdump(const void* buf, size_t len) {
		string ret("\r\n");
		char tmp[8];
		const uint8_t* data = (const uint8_t*)buf;
		for (size_t i = 0; i < len; i += 16) {
			for (int j = 0; j < 16; ++j) {
				if (i + j < len) {
					int sz = snprintf(tmp, sizeof(tmp), "%.2x ", data[i + j]);
					ret.append(tmp, sz);
				}
				else {
					int sz = snprintf(tmp, sizeof(tmp), "   ");
					ret.append(tmp, sz);
				}
			}
			for (int j = 0; j < 16; ++j) {
				if (i + j < len) {
					ret += (is_safe(data[i + j]) ? data[i + j] : '.');
				}
				else {
					ret += (' ');
				}
			}
			ret += ('\n');
		}
		return ret;
	}

	string hexmem(const void* buf, size_t len) {
		string ret;
		char tmp[8];
		const uint8_t* data = (const uint8_t*)buf;
		for (size_t i = 0; i < len; ++i) {
			int sz = sprintf_s(tmp, "%.2x ", data[i]);
			ret.append(tmp, sz);
		}
		return ret;
	}

	// string转小写
	std::string& strToLower(std::string& str) {
		transform(str.begin(), str.end(), str.begin(), towlower);
		return str;
	}

	// string转大写
	std::string& strToUpper(std::string& str) {
		transform(str.begin(), str.end(), str.begin(), towupper);
		return str;
	}

	// string转小写
	std::string strToLower(std::string&& str) {
		transform(str.begin(), str.end(), str.begin(), towlower);
		return std::move(str);
	}

	// string转大写
	std::string strToUpper(std::string&& str) {
		transform(str.begin(), str.end(), str.begin(), towupper);
		return std::move(str);
	}

	vector<string> split(const string& s, const char* delim) {
		vector<string> ret;
		size_t last = 0;
		auto index = s.find(delim, last);
		while (index != string::npos) {
			if (index - last > 0) {
				ret.push_back(s.substr(last, index - last));
			}
			last = index + strlen(delim);
			index = s.find(delim, last);
		}
		if (!s.size() || s.size() - last > 0) {
			ret.push_back(s.substr(last));
		}
		return ret;
	}

#define TRIM(s, chars) \
do{ \
    string map(0xFF, '\0'); \
    for (auto &ch : chars) { \
        map[(unsigned char &)ch] = '\1'; \
    } \
    while( s.size() && map.at((unsigned char &)s.back())) s.pop_back(); \
    while( s.size() && map.at((unsigned char &)s.front())) s.erase(0,1); \
}while(0);

	//去除前后的空格、回车符、制表符
	std::string& trim(std::string& s, const string& chars) {
		TRIM(s, chars);
		return s;
	}

	std::string trim(std::string&& s, const string& chars) {
		TRIM(s, chars);
		return std::move(s);
	}

	void replace(string& str, const string& old_str, const string& new_str) {
		if (old_str.empty() || old_str == new_str) {
			return;
		}
		auto pos = str.find(old_str);
		if (pos == string::npos) {
			return;
		}
		str.replace(pos, old_str.size(), new_str);
		replace(str, old_str, new_str);
	}

	bool start_with(const string& str, const string& substr) {
		return str.find(substr) == 0;
	}

	bool end_with(const string& str, const string& substr) {
		auto pos = str.rfind(substr);
		return pos != string::npos && pos == str.size() - substr.size();
	}


#if defined(_WIN32)
	void sleep(int second) {
		Sleep(1000 * second);
	}

	const char* strcasestr(const char* big, const char* little) {
		string big_str = big;
		string little_str = little;
		strToLower(big_str);
		strToLower(little_str);
		auto pos = strstr(big_str.data(), little_str.data());
		if (!pos) {
			return nullptr;
		}
		return big + (pos - big_str.data());
	}

	int vasprintf(char** strp, const char* fmt, va_list ap) {
		// _vscprintf tells you how big the buffer needs to be
		int len = _vscprintf(fmt, ap);
		if (len == -1) {
			return -1;
		}
		size_t size = (size_t)len + 1;
		char* str = (char*)malloc(size);
		if (!str) {
			return -1;
		}
		// _vsprintf_s is the "secure" version of vsprintf
		int r = vsprintf_s(str, len + 1, fmt, ap);
		if (r == -1) {
			free(str);
			return -1;
		}
		*strp = str;
		return r;
	}

	int asprintf(char** strp, const char* fmt, ...) {
		va_list ap;
		va_start(ap, fmt);
		int r = vasprintf(strp, fmt, ap);
		va_end(ap);
		return r;
	}

#endif //WIN32


	static bool SetConsoleColor(WORD Color)
	{
		HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
		if (handle == 0)
			return false;

		BOOL ret = SetConsoleTextAttribute(handle, Color);
		return(ret == TRUE);
	}

	bool SetConsoleColorInfo() {
		return SetConsoleColor(FOREGROUND_GREEN);
	}

	bool SetConsoleColorError() {
		return SetConsoleColor(FOREGROUND_RED);
	}

	bool SetConsoleColorDebug() {
		return SetConsoleColor(FOREGROUND_BLUE);
	}

	bool SetConsoleColorYellow() {
		return SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN);
	}

	bool RestoreConsoleColor() {
		return SetConsoleColor(0x0007);
	}

}  // namespace toolkit

