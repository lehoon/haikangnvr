#include "DeviceWorker.h"
#include "common_define.h"

#include "Device.h"

#include <windows.h>
#include <process.h>

DeviceWorker::DeviceWorker(connectInfo * info) {
	connectInfo_.strDevIp = info->strDevIp;
	connectInfo_.ui16DevPort = info->ui16DevPort;
	connectInfo_.strUserName = info->strUserName;
	connectInfo_.strPwd = info->strPwd;
	//deviceHkPtr = std::make_shared<DeviceHK>(new DeviceHK);
}

DeviceWorker::~DeviceWorker() {

}

bool DeviceWorker::InitTask() {
	return true;
}

bool DeviceWorker::ExitTask() {
	return true;
}

bool DeviceWorker::Work() {
	deviceHkPtr->connectDevice(connectInfo_, [&](bool success, const connectResult& result) {
		if (!success) {
			CONSOLE_COLOR_ERROR();
			std::cout << "海康设备链接失败,请检查IP地址、用户名密码是否正确." << std::endl;
			return;
		}

		CONSOLE_COLOR_INFO();
		std::cout << "链接返回结果" << success
			<< ",设备名称:"
			<< result.strDevName
			<< ",通道起始编号:"
			<< result.ui16ChnStart
			<< ",通道数:"
			<< result.ui16ChnCount
			<< std::endl;

		//添加ip通道信息
		//devicePtr->addAllChannel(true);
		});


	return true;
}

std::string DeviceWorker::Name() const {
	return "device_worker";
}

unsigned int DeviceWorker::threadFunc(void* p)
{
	DeviceWorker* threadObj = (DeviceWorker*)p;
	return threadObj->Work();
}

bool DeviceWorker::StartThread()
{
	quitEt_.ResetEvent();
	suspendEt_.ResetEvent();
	threadHandle_ = (HANDLE)_beginthreadex(NULL, 0,
		DeviceWorker::threadFunc, (void*)this, 0, &threadId_);
	std::cout << "{} thread start successfully, id:{}" << Name() << "," << _threadid << std::endl;
	return threadHandle_ != 0;
}
bool DeviceWorker::StopThread()
{
	if (NULL != threadHandle_) {
		//LOG_DEBUG("stop thread... with id:" << threadId_);
		quitEt_.SetEvent();
		WaitForSingleObject(threadHandle_, 100);
		CloseHandle(threadHandle_);
		threadHandle_ = NULL;
		//LOG_DEBUG("stop success.");
	}
	//LOG_DEBUG("work thread stop successfully.");
	std::cout << "{} work thread stop successfully." << Name() << std::endl;
	return TRUE;
}
