#pragma once

#include "socket_context_pool_class.h"

SocketContextPool::SocketContextPool()
{
	CSAutoLock cs(m_csLock);
	nConnectionSocket = 0;
	std::cout << "SocketContertPool 初始化完成!\n";
}

SocketContextPool::~SocketContextPool()
{
	CSAutoLock cs(m_csLock);
}

LPPER_SOCKET_CONTEXT SocketContextPool::AllocateSocketContext()
{
	CSAutoLock cs(m_csLock);
	LPPER_SOCKET_CONTEXT psocket = SocketPool.newElement();
	nConnectionSocket++;
	
	return psocket;
}

void SocketContextPool::ReleaseSocketContext(LPPER_SOCKET_CONTEXT pSocket)
{
	CSAutoLock cs(m_csLock);
	SocketPool.deleteElement(pSocket);
	nConnectionSocket--;
}
unsigned int SocketContextPool::NumOfConnectingServer()
{
	return nConnectionSocket;
}