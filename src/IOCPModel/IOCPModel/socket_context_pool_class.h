#pragma once

#include"memory_pool_class.h"
#include"memory_pool_class.cpp"
#include"per_socket_context_struct.h"
#include<iostream>

//========================================================
//                 
//		  SocketContextPool，单例SocketContext池
//
//========================================================

class SocketContextPool
{
private:
	MemoryPool<_PER_SOCKET_CONTEXT, 102400> SocketPool;
	unsigned int nConnectionSocket;

	CRITICAL_SECTION csLock;
public:
	SocketContextPool();
	~SocketContextPool();

	// 分配一个新的SocketContext
	LPPER_SOCKET_CONTEXT AllocateSocketContext();

	// 回收一个SocketContext
	void ReleaseSocketContext(LPPER_SOCKET_CONTEXT pSocket);

	// 正在处于连接状态的SocketContext数量
	unsigned int NumOfConnectingServer();
};
