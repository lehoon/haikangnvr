
#pragma once

#include <windows.h>
#include "IThreadWrap.h"
#include "Event.h"

class WorkThread : public IThreadWrap
{
public:
	WorkThread();
	WorkThread(IThreadUser* pWorker);
	virtual ~WorkThread();

	//thread interface
	virtual bool StartThread();
	virtual bool StopThread();

	static unsigned int WINAPI threadFunc(void* p);
	unsigned int DoWork();

public:
	void SetThreadUser(IThreadUser*);

private:
	IThreadUser* worker_;
	std::string worker_name_;
};

