#ifndef HAIKANG_NVR_TESTING_COMMON_DEFINE_H_
#define HAIKANG_NVR_TESTING_COMMON_DEFINE_H_

#include "HCNetSDK.h"
#include "PlayM4.h"
#include "Logger.h"
#include "struct_pinter.h"

#include <functional>
#include <iostream>
#include <string>
#include <sstream>


class DeviceAuth {
public:
	DeviceAuth() {}
	DeviceAuth(std::string host, std::string username, std::string password, unsigned short port) : 
		_host(std::move(host)),
		_username(std::move(username)),
		_password(std::move(password)),
		_port(port)
	{

	}
	~DeviceAuth() {}

public:
	std::string _host;
	std::string _username;
	std::string _password;
	unsigned short _port;
};


#define StrPrinter _StrPrinter()
class _StrPrinter : public std::string {
public:
	_StrPrinter() {}

	template<typename T>
	_StrPrinter& operator <<(T&& data) {
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


class onceToken {
public:
	using task = std::function<void(void)>;

	template<typename FUNC>
	onceToken(const FUNC& onConstructed, task onDestructed = nullptr) {
		onConstructed();
		_onDestructed = std::move(onDestructed);
	}

	onceToken(std::nullptr_t, task onDestructed = nullptr) {
		_onDestructed = std::move(onDestructed);
	}

	~onceToken() {
		if (_onDestructed) {
			_onDestructed();
		}
	}

private:
	onceToken() = delete;
	onceToken(const onceToken&) = delete;
	onceToken(onceToken&&) = delete;
	onceToken& operator=(const onceToken&) = delete;
	onceToken& operator=(onceToken&&) = delete;

private:
	task _onDestructed;
};



#endif //HAIKANG_NVR_TESTING_COMMON_DEFINE_H_