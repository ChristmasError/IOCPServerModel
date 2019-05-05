#pragma once

#include<memory_pool_class.h>
#include<memory_pool_class.cpp>
#include<socket_context_struct.cpp>
#include<iostream>

class SocketContextPool
{
private:
	MemoryPool<_PER_SOCKET_CONTEXT, 102400> SocketPool;
	unsigned int nConnectionSocket;
	CRITICAL_SECTION csLock;
public:
	SocketContextPool()
	{
		InitializeCriticalSection(&csLock);
		nConnectionSocket = 0;
		std::cout << "SocketContert 构造完成！\n";
	}
	~SocketContextPool()
	{
		EnterCriticalSection(&csLock);

		LeaveCriticalSection(&csLock);
	}

	LPPER_SOCKET_CONTEXT newSocketContext()
	{
		EnterCriticalSection(&csLock);
		LPPER_SOCKET_CONTEXT psocket = SocketPool.newElement();
		nConnectionSocket++;
		LeaveCriticalSection(&csLock);

		return psocket;
	}
	void deleteSocketContext(LPPER_SOCKET_CONTEXT psocket)
	{
		EnterCriticalSection(&csLock);
		SocketPool.deleteElement(psocket);
		nConnectionSocket--;
		LeaveCriticalSection(&csLock);
	}

};
