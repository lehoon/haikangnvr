#ifndef HAIKANG_NVR_WORK_THREAD_WRAP_H_
#define HAIKANG_NVR_WORK_THREAD_WRAP_H_


#include "Event.h"

class IThreadWrap
{
public:
	IThreadWrap() : threadHandle_(NULL), threadId_(0) {}
	virtual bool StartThread() = 0;
	virtual bool StopThread() = 0;
	virtual bool SuspendThread() {
		suspendEt_.SetEvent();
		if (::WaitForSingleObject(suspendOKEt_, 2000) == WAIT_TIMEOUT) {
			//log
			return FALSE;
		}
		return TRUE;
	}
	virtual void ResumeThread() {
		//ASSERT_NOT_NULL(threadHandle_);
		::ResumeThread(threadHandle_);
	}

protected:
	base::Event suspendEt_;
	base::Event suspendOKEt_;
	base::Event quitEt_;
	HANDLE threadHandle_;
	unsigned int threadId_;
};

class IThreadUser
{
public:
	virtual bool InitTask() = 0;
	virtual bool ExitTask() = 0;
	virtual bool Work() = 0;
	virtual std::string Name() const = 0;
};

#endif //HAIKANG_NVR_WORK_THREAD_WRAP_H_