
#include "common_define.h"
#include "WorkThread.h"
#include <process.h>

WorkThread::WorkThread() {
	threadHandle_ = nullptr;
	threadId_ = 0;
	worker_ = nullptr;
}

WorkThread::WorkThread(IThreadUser* pWorker)
{
	threadHandle_ = NULL;
	threadId_ = 0;
	worker_ = pWorker;
	worker_name_ = pWorker->Name();
}
WorkThread::~WorkThread()
{
	StopThread();
}

void WorkThread::SetThreadUser(IThreadUser* threadUser) {
	worker_ = threadUser;
	worker_name_ = threadUser->Name();
}

bool WorkThread::StartThread()
{
	if (worker_ == nullptr) {
		std::cout<<"StartThread failed, _worker is nullptr"<<std::endl;
		return false;
	}

	//ASSERT_NOT_NULL(worker_);
	if (!worker_->InitTask()) {
		std::cout << "InitTask failed " << worker_->Name() << std::endl;;
		return false;
	}

	quitEt_.ResetEvent();
	suspendEt_.ResetEvent();
	threadHandle_ = (HANDLE)_beginthreadex(NULL, 0,
		WorkThread::threadFunc, (void*)this, 0, &threadId_);
	std::cout<<"{} thread start successfully, id:{}" << worker_->Name() << ","<< _threadid << std::endl;
	return threadHandle_ != 0;
}
bool WorkThread::StopThread()
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
	std::cout << "{} work thread stop successfully." << worker_name_ << std::endl;
	return TRUE;
}
unsigned int WorkThread::threadFunc(void* p)
{
	WorkThread* threadObj = (WorkThread*)p;
	return threadObj->DoWork();
}
unsigned int WorkThread::DoWork()
{
	HANDLE hEvts[2];
	hEvts[0] = quitEt_;
	hEvts[1] = suspendEt_;
	while (TRUE) {
		DWORD dwRet = WaitForMultipleObjects(2, hEvts, FALSE, 0);
		dwRet -= WAIT_OBJECT_0;
		if (dwRet == 0) {
			//LOG_INFO("got a quit single, so the thread exit.");
			std::cout << "recive the exit sigal now exit" << worker_name_ << std::endl;
			break;
		}
		if (dwRet == 1) {
			suspendOKEt_.SetEvent();
			::SuspendThread(threadHandle_);
			continue;
		}

		if (!worker_->Work()) {
			std::cout << "{} thread now exit" << worker_name_ << std::endl;
			break;
		}
	}

	worker_->ExitTask();
	_endthreadex(0);
	return 0;
}

