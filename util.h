/*
 * Copyright (c) 2016 The ZLToolKit project authors. All Rights Reserved.
 *
 * This file is part of ZLToolKit(https://github.com/ZLMediaKit/ZLToolKit).
 *
 * Use of this source code is governed by MIT license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#ifndef UTIL_UTIL_H_
#define UTIL_UTIL_H_

#include <ctime>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <sstream>
#include <vector>
#include <atomic>
#include <unordered_map>
#include <windows.h>


#define INSTANCE_IMP(class_name, ...) \
class_name &class_name::Instance() { \
    static std::shared_ptr<class_name> s_instance(new class_name(__VA_ARGS__)); \
    static class_name &s_instance_ref = *s_instance; \
    return s_instance_ref; \
}

namespace toolkit {
	class AppendString : public std::string {
	public:
		AppendString() {}

		template<typename T>
		AppendString& operator <<(T&& data) {
			_stream << std::forward<T>(data);
			this->std::string::operator=(_stream.str());
			return *this;
		}

		std::string operator <<(std::ostream& (*f)(std::ostream&)) const {
			return *this;
		}

	private:
		std::stringstream _stream;
	};


	//禁止拷贝基类
	class noncopyable {
	protected:
		noncopyable() {}
		~noncopyable() {}
	private:
		//禁止拷贝
		noncopyable(const noncopyable& that) = delete;
		noncopyable(noncopyable&& that) = delete;
		noncopyable& operator=(const noncopyable& that) = delete;
		noncopyable& operator=(noncopyable&& that) = delete;
	};

	//可以保存任意的对象
	class Any {
	public:
		using Ptr = std::shared_ptr<Any>;

		Any() = default;
		~Any() = default;

		template <typename C, typename ...ArgsType>
		void set(ArgsType &&...args) {
			_data.reset(new C(std::forward<ArgsType>(args)...), [](void* ptr) {
				delete (C*)ptr;
				});
		}
		template <typename C>
		C& get() {
			if (!_data) {
				throw std::invalid_argument("Any is empty");
			}
			C* ptr = (C*)_data.get();
			return *ptr;
		}

		operator bool() {
			return _data.operator bool();
		}
		bool empty() {
			return !bool();
		}
	private:
		std::shared_ptr<void> _data;
	};

	//用于保存一些外加属性
	class AnyStorage : public std::unordered_map<std::string, Any> {
	public:
		AnyStorage() = default;
		~AnyStorage() = default;
		using Ptr = std::shared_ptr<AnyStorage>;
	};

	//对象安全的构建和析构
	//构建后执行onCreate函数
	//析构前执行onDestory函数
	//在函数onCreate和onDestory中可以执行构造或析构中不能调用的方法，比如说shared_from_this或者虚函数
	class Creator {
	public:
		template<typename C, typename ...ArgsType>
		static std::shared_ptr<C> create(ArgsType &&...args) {
			std::shared_ptr<C> ret(new C(std::forward<ArgsType>(args)...), [](C* ptr) {
				ptr->onDestory();
				delete ptr;
				});
			ret->onCreate();
			return ret;
		}
	private:
		Creator() = default;
		~Creator() = default;
	};


	template <class C>
	class ObjectStatistic {
	public:
		ObjectStatistic() {
			++getCounter();
		}

		~ObjectStatistic() {
			--getCounter();
		}

		static size_t count() {
			return getCounter().load();
		}

	private:
		static std::atomic<size_t>& getCounter();
	};

#define StatisticImp(Type)  \
    template<> \
    std::atomic<size_t>& ObjectStatistic<Type>::getCounter(){ \
        static std::atomic<size_t> instance(0); \
        return instance; \
    }

	std::string makeRandStr(int sz, bool printable = true);
	std::string hexdump(const void* buf, size_t len);
	std::string hexmem(const void* buf, size_t len);
	std::string exePath(bool isExe = true);
	std::string exeDir(bool isExe = true);
	std::string exeName(bool isExe = true);

	std::vector<std::string> split(const std::string& s, const char* delim);
	//去除前后的空格、回车符、制表符...
	std::string& trim(std::string& s, const std::string& chars = " \r\n\t");
	std::string trim(std::string&& s, const std::string& chars = " \r\n\t");
	// string转小写
	std::string& strToLower(std::string& str);
	std::string strToLower(std::string&& str);
	// string转大写
	std::string& strToUpper(std::string& str);
	std::string strToUpper(std::string&& str);
	//替换子字符串
	void replace(std::string& str, const std::string& old_str, const std::string& new_str);
	//判断是否为ip
	bool isIP(const char* str);
	//字符串是否以xx开头
	bool start_with(const std::string& str, const std::string& substr);
	//字符串是否以xx结尾
	bool end_with(const std::string& str, const std::string& substr);
	//拼接格式字符串
	template<typename... Args>
	std::string str_format(const std::string& format, Args... args);

#ifndef bzero
#define bzero(ptr,size)  memset((ptr),0,(size));
#endif //bzero

	bool SetConsoleColorError();
	bool SetConsoleColorInfo();
	bool SetConsoleColorDebug();
	bool RestoreConsoleColor();
	bool SetConsoleColorYellow();


}  // namespace toolkit
#endif /* UTIL_UTIL_H_ */
