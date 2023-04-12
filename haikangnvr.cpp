// haikangnvr.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include "common_define.h"
#include "struct_pinter.h"
#include "semaphore.h"
#include "Configure.h"
#include "Device.h"
#include "Helper.h"
#include "util.h"

#include "client/crash_report_database.h"
#include "client/settings.h"
#include "client/crashpad_client.h"

#include <cmdline.h>
#include <vector>
#include <iostream>
#include <signal.h>

#if defined (__cplusplus)
extern "C" {
#include "x264.h"
}
#else
#include "x264.h"
#endif

void InitCrashpad() {
	using namespace crashpad;
	std::map<std::string, std::string> annotations;
	std::vector<std::string> arguments;
	std::string url = "http://127.0.0.1:5555";
	arguments.push_back("--no-rate-limit");
	base::FilePath db(L"./");
	base::FilePath handler(L"./crashpad_handler.exe");

	std::unique_ptr<CrashReportDatabase> database =
		crashpad::CrashReportDatabase::Initialize(db);

	if (database == nullptr || database->GetSettings() == nullptr) {
		return;
	}

	database->GetSettings()->SetUploadsEnabled(true);
	CrashpadClient client;
	bool ret = client.StartHandler(handler,
		db,
		db,
		url,
		annotations,
		arguments,
		true,
		true);

	if (!ret) {
		return;
	}

	ret = client.WaitForHandlerStart(INFINITE);
	if (!ret) {
		return;
	}

	return;
}

#if 0
int main(int argc, char* argv[]) {
	x264_param_t x264_param;
	x264_param_default(&x264_param);
	std::cout << x264_param.b_cpu_independent << std::endl;
	return 0;
}
#endif

int main(int argc, char* argv[])
{
	InitCrashpad();
	cmdline::parser cmd_parser;

	cmd_parser.add<int>("count", 'c', "测试设备个数, 默认:10", false, 10);
	cmd_parser.add<int>("frame", 'f', "解码帧间隔, 默认:8帧", false, 8);
	cmd_parser.add("decode", 'd', "是否启用转码, 默认不转码");
	cmd_parser.add("mat", 'm', "是否生成mat对象,依赖转码参数, 默认不生成mat对象");
	cmd_parser.add("logger", 'l', "是否记录解码日志, 验证数据使用,默认不打印日志");
	cmd_parser.add("print", 'p', "打印设备信息");
	cmd_parser.add("help", 'h', "打印使用帮助信息");

	//解析命令行参数
	cmd_parser.parse_check(argc, argv);

	if (cmd_parser.exist("help")) {
		std::cout << cmd_parser.usage();
		return 0;
	}

	CConfigure config;
	if (config.Init("./config.ini")) {
		std::cout << "当前目录下未找到配置文件config.ini,程序退出." << std::endl;
		return -1;
	}

	bool bDeviceInfoPrint = false;
	if (cmd_parser.exist("print")) {
		CONSOLE_COLOR_INFO();
		std::cout << "打印设备信息,请稍后." << std::endl << std::endl;
		bDeviceInfoPrint = true;
	}

	if (!NET_DVR_Init()) {
		DWORD errCode = NET_DVR_GetLastError();
		CONSOLE_COLOR_ERROR();
		std::cout << "初始化海康sdk环境失败, code=" << errCode << ", message = " << NET_DVR_GetErrorMsg((LONG*)&errCode) << std::endl;
		CONSOLE_COLOR_RESET();
		return -1;
	}

	//设置日志路径
	NET_DVR_SetLogToFile(3, (char*)"logs");

	std::string device_host = config.GetValueAsString("haikang", "device.host", "172.17.18.233");
	unsigned short device_port = config.GetValueAsUint32("haikang", "device.port", 8000);
	std::string device_username = config.GetValueAsString("haikang", "device.username", "admin");
	std::string device_password = config.GetValueAsString("haikang", "device.password", "Effort@2022");

	CONSOLE_COLOR_YELLOW();
	std::cout << "接入视频设备地址:"
		<< "" << device_host
		<< "," << device_port
		<< "," << device_username
		<< "," << device_password
		<< std::endl
		<< std::endl;

	SPDLOG_INFO("接入视频设备地址: {}, {}, {}, {}", device_host, device_port, device_username, device_password);

	if (bDeviceInfoPrint) {
		DeviceInfoHK::Ptr deviceInfoPtr(new DeviceInfoHK);
		connectInfo connectInfo(device_host, device_port, device_username, device_password, 10, false);
		deviceInfoPtr->connectDevice(connectInfo, [&](bool success, const connectResult& result) {});
		SPDLOG_INFO("打印设备信息完成.");
		CONSOLE_COLOR_RESET();
		return 0;
	}

	//帧数   字节数
	int64_t i64frame_count = 0, i64byte_count = 0;

	//测试参数
	unsigned int device_count = cmd_parser.get<int>("count");
	bool device_decode = cmd_parser.exist("decode");
	bool device_mat = cmd_parser.exist("mat");
	bool device_logger_open = cmd_parser.exist("logger");
	unsigned int frame_count = cmd_parser.get<int>("frame");

	if (device_count <= 0 || device_count >= 1024) device_count = 10;
	if (frame_count <= 0 || frame_count >= 60 * 60) frame_count = 8;

	CONSOLE_COLOR_INFO();
	std::cout << "测试参数:" << std::endl;
	std::cout << "	接入视频数:"
		<< device_count
		<< "路"
		<< std::endl
		<< "	解码间隔:"
		<< frame_count
		<< "帧"
		<< std::endl
		<< "	是否转码:"
		<< Helper::decode_name(device_decode)
		<< std::endl
		<< "	是否生成mat对象:"
		<< Helper::decode_name(device_mat)
		<< std::endl
		<< "	是否打印视频流数据:"
		<< Helper::logger_open(device_logger_open)
		<< std::endl
		<< std::endl;

	toolkit::AppendString append_string;
	append_string << "接入视频数:"
		<< device_count
		<< "路,解码间隔:"
		<< frame_count
		<< "帧,是否转码:"
		<< Helper::decode_name(device_decode)
		<< ",是否生成mat对象:"
		<< Helper::decode_name(device_mat)
		<< ",是否打印视频流数据:"
		<< Helper::logger_open(device_logger_open);

	SPDLOG_INFO("测试参数:{}", append_string);
	SPDLOG_INFO("接入视频设备地址: {}, {}, {}, {}", device_host, device_port, device_username, device_password);

	long device_play_success = 0;
	long time_begin = clock();

	{
		//frame_count *= 24;
		connectInfo connectInfo(device_host, device_port, device_username, device_password, frame_count, device_logger_open);

		if (device_decode && device_mat) {
			DecodeToMatDeviceHK::Ptr devicePtr(new DecodeToMatDeviceHK(device_count));
			if (!devicePtr->connectDevice(connectInfo, nullptr)) {
				CONSOLE_COLOR_ERROR();
				std::cout << "海康设备登录失败,请检查IP地址、用户名密码是否正确." << std::endl;
				SPDLOG_ERROR("海康设备登录失败,请检查IP地址、用户名密码是否正确.");
				CONSOLE_COLOR_RESET();
				return -1;
			}

			//设置退出信号处理函数
			static toolkit::semaphore sem;
			signal(SIGINT, [](int) {
				signal(SIGINT, SIG_IGN);// 设置退出信号
				sem.post();
				});// 设置退出信号

			//获取成功接入视频路数
			device_play_success = devicePtr->channelCount();
			std::cout << "海康设备视频流接入测试,本次登陆设备成功数:" << 1 << ",接入实时视频流通道数:" << device_play_success << std::endl;
			CONSOLE_COLOR_DEBUG();
			std::cout << std::endl << "测试中,请稍后,使用CTRL+C停止测试程序" << std::endl;
			sem.wait();
		}
		else if (device_decode) {
			DecodeDeviceHK::Ptr devicePtr(new DecodeDeviceHK(device_count));
			if (!devicePtr->connectDevice(connectInfo, nullptr)) {
				CONSOLE_COLOR_ERROR();
				std::cout << "海康设备登录失败,请检查IP地址、用户名密码是否正确." << std::endl;
				SPDLOG_ERROR("海康设备登录失败,请检查IP地址、用户名密码是否正确.");
				CONSOLE_COLOR_RESET();
				return -1;
			}

			//设置退出信号处理函数
			static toolkit::semaphore sem;
			signal(SIGINT, [](int) {
				signal(SIGINT, SIG_IGN);// 设置退出信号
				sem.post();
				});// 设置退出信号

			//获取成功接入视频路数
			device_play_success = devicePtr->channelCount();
			std::cout << "海康设备视频流接入测试,本次登陆设备成功数:" << 1 << ",接入实时视频流通道数:" << device_play_success << std::endl;
			CONSOLE_COLOR_DEBUG();
			std::cout << std::endl << "测试中,请稍后,使用CTRL+C停止测试程序" << std::endl;
			sem.wait();
		}
		else {
			DeviceHK::Ptr devicePtr(new DeviceHK(device_count));
			if (!devicePtr->connectDevice(connectInfo, nullptr)) {
				CONSOLE_COLOR_ERROR();
				std::cout << "海康设备登录失败,请检查IP地址、用户名密码是否正确." << std::endl;
				SPDLOG_ERROR("海康设备登录失败,请检查IP地址、用户名密码是否正确.");
				CONSOLE_COLOR_RESET();
				return -1;
			}

			//设置退出信号处理函数
			static toolkit::semaphore sem;
			signal(SIGINT, [](int) {
				signal(SIGINT, SIG_IGN);// 设置退出信号
				sem.post();
				});// 设置退出信号

			//获取成功接入视频路数
			device_play_success = devicePtr->channelCount();
			std::cout << "海康设备视频流接入测试,本次登陆设备成功数:" << 1 << ",接入实时视频流通道数:" << device_play_success << std::endl;
			CONSOLE_COLOR_DEBUG();
			std::cout << std::endl << "测试中,请稍后,使用CTRL+C停止测试程序" << std::endl;
			sem.wait();
		}
	}
	long time_end = clock();
	CONSOLE_COLOR_YELLOW();
	std::cout << "接收到ctrl+c退出信号,程序准备退出" << std::endl;
	SPDLOG_INFO("接收到ctrl+c退出信号,程序准备退出");
	Sleep(3000);

	CONSOLE_COLOR_INFO();
	SPDLOG_INFO("海康设备视频流接入测试完成,本次登陆设备成功数:{},接入实时视频流通道数:{}", 1, device_play_success);
	std::cout << "海康设备视频流接入测试完成,本次登陆设备成功数:" << 1 << ",接入实时视频流通道数:" << device_play_success << std::endl;

	long time_sub_second = (time_end - time_begin) / CLOCKS_PER_SEC;
	SPDLOG_INFO("本次测试时间:{}秒", time_sub_second);
	std::cout << "本次测试时间:" << time_sub_second << "秒" << std::endl;

	CONSOLE_COLOR_RESET();
	return 0;
}