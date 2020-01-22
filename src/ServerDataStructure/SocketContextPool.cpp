#pragma once

#include "SocketContextPool.h"

IOContextPool _PER_SOCKET_CONTEXT::ioContextPool;

SocketContextPool::SocketContextPool()
{
	nConnectionSocket = 0;
	std::cout << "SocketContertPool初始化完成...\n";
}

SocketContextPool::~SocketContextPool()
{

}

LPPER_SOCKET_CONTEXT SocketContextPool::AllocateSocketContext()
{
	CSAutoLock lock(m_csLock);
	LPPER_SOCKET_CONTEXT psocket = SocketPool.newElement();
	nConnectionSocket++;
	
	return psocket;
}

void SocketContextPool::ReleaseSocketContext(LPPER_SOCKET_CONTEXT pSocket)
{
	SocketPool.deleteElement(pSocket);
	nConnectionSocket--;
}
unsigned int SocketContextPool::NumOfConnectingServer()
{

	return nConnectionSocket;
}