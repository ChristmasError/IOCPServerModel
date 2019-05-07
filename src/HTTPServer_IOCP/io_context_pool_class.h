#pragma once

#include<per_io_context_struct.cpp>

//========================================================
//                 
//		       IOContextPool，单例IOContext池
//
//========================================================

class IOContextPool
{
private:
	std::list<LPPER_IO_CONTEXT> contextList;
	CRITICAL_SECTION csLock;

public:
	IOContextPool();
	~IOContextPool();

	// 分配一个IOContxt
	LPPER_IO_CONTEXT AllocateIoContext();
	
	// 回收一个IOContxt
	void ReleaseIOContext(LPPER_IO_CONTEXT pContext);

};


