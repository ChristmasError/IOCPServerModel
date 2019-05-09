#pragma once

#include<per_io_context_struct.h>
#include<winsock_class.h>
#include<io_context_pool_class.h>
#include<vector>

//====================================================================================
//
//				单句柄数据结构体定义(用于每一个完成端口，也就是每一个Socket的参数)
//
//====================================================================================

typedef struct _PER_SOCKET_CONTEXT
{
private:
	static IOContextPool			 ioContextPool;		  // 空闲的IOcontext池
private:
	std::vector<LPPER_IO_CONTEXT>	 arrIoContext;		  // 同一个socket上的多个IO请求
	CRITICAL_SECTION				 csLock;
public:
	WinSock		m_Sock;		//每一个socket的信息

	// 初始化
	_PER_SOCKET_CONTEXT()
	{
		m_Sock.socket = INVALID_SOCKET;
	}
	// 释放资源
	~_PER_SOCKET_CONTEXT()
	{
		for (auto it : arrIoContext)
		{
			ioContextPool.ReleaseIOContext(it);
		}
		EnterCriticalSection(&csLock);
		arrIoContext.clear();
		LeaveCriticalSection(&csLock);

		DeleteCriticalSection(&csLock);
	}
	// 获取一个新的IO_DATA
	LPPER_IO_CONTEXT GetNewIOContext()
	{
		LPPER_IO_CONTEXT context = ioContextPool.AllocateIoContext();
		if (context != NULL)
		{
			EnterCriticalSection(&csLock);
			arrIoContext.push_back(context);
			LeaveCriticalSection(&csLock);
		}
		return context;
	}
	// 从数组中移除一个指定的IoContext
	void RemoveIOContext(LPPER_IO_CONTEXT pContext)
	{
		for (std::vector<LPPER_IO_CONTEXT>::iterator it = arrIoContext.begin(); it != arrIoContext.end(); it++)
		{
			if (pContext == *it)
			{
				ioContextPool.ReleaseIOContext(*it);

				EnterCriticalSection(&csLock);
				arrIoContext.erase(it);
				LeaveCriticalSection(&csLock);

				break;
			}
		}
	}

} PER_SOCKET_CONTEXT, *LPPER_SOCKET_CONTEXT;


