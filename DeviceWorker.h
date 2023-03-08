#pragma once

#include "common_define.h"
#include "IThreadWrap.h"

#include "Device.h"

class DeviceWorker : public IThreadWrap, public std::enable_shared_from_this<DeviceWorker>
{
public:
	DeviceWorker(connectInfo *);
	~DeviceWorker();
	using Ptr = std::shared_ptr<DeviceWorker>;

	//thread interface
	virtual bool StartThread();
	virtual bool StopThread();

public:
	bool InitTask();
	bool ExitTask();
	bool Work();
	std::string Name() const;
	static unsigned int WINAPI threadFunc(void* p);


protected:
	connectInfo connectInfo_;
	DeviceHK::Ptr deviceHkPtr;
};

