#pragma once

#include "../MemoryPool/MemoryPool.h"
#include "PerSocketContext.h"

//========================================================
//                 
//		  SocketContextPool，单例SocketContext池
//
//========================================================

class SERVERDATASTRUCTURE_API SocketContextPool
{
private:
	MemoryPool<_PER_SOCKET_CONTEXT, 102400> SocketPool;
	unsigned int nConnectionSocket;

	CSLock m_csLock;
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
