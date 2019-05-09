#pragma once

#include "socket_context_pool_class.h"

SocketContextPool::SocketContextPool()
{
	InitializeCriticalSection(&csLock);
	nConnectionSocket = 0;
	std::cout << "SocketContertPool 初始化完成!\n";
}

SocketContextPool::~SocketContextPool()
{
	DeleteCriticalSection(&csLock);
}

LPPER_SOCKET_CONTEXT SocketContextPool::AllocateSocketContext()
{
	EnterCriticalSection(&csLock);
	LPPER_SOCKET_CONTEXT psocket = SocketPool.newElement();
	nConnectionSocket++;
	LeaveCriticalSection(&csLock);

	return psocket;
}

void SocketContextPool::ReleaseSocketContext(LPPER_SOCKET_CONTEXT pSocket)
{
	EnterCriticalSection(&csLock);
	SocketPool.deleteElement(pSocket);
	nConnectionSocket--;
	LeaveCriticalSection(&csLock);
}
unsigned int SocketContextPool::NumOfConnectingServer()
{
	return nConnectionSocket;
}