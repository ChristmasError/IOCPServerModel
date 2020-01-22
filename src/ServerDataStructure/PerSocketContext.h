#pragma once

#include "serverdatastructure_global.h"
#include "PerIOContext.h"
#include "IOContextPool.h"
#include "../WinSocket/WinSocket.h"

//====================================================================================
//
//				单句柄数据结构体定义(用于每一个完成端口，也就是每一个Socket的参数)
//
//====================================================================================

typedef class SERVERDATASTRUCTURE_API _PER_SOCKET_CONTEXT
{
public:
	WinSocket	 m_Sock;		//每一个socket的信息
private:
	static IOContextPool			 ioContextPool;		  // 空闲的IOcontext池
	CRITICAL_SECTION                 m_csLock;			  // 互斥事件
	std::vector<LPPER_IO_CONTEXT>	 arrIoContext;		  // 同一个socket上的多个IO请求

public:
	_PER_SOCKET_CONTEXT()
	{
		InitializeCriticalSection(&m_csLock);
		m_Sock.socket = INVALID_SOCKET;	
	}
	// 释放资源
	~_PER_SOCKET_CONTEXT()
	{
		for (std::vector<LPPER_IO_CONTEXT>::iterator it = arrIoContext.begin(); it != arrIoContext.end(); it++)
		{
			ioContextPool.ReleaseIOContext(*it);
		}
		EnterCriticalSection(&m_csLock);
		arrIoContext.clear();
		LeaveCriticalSection(&m_csLock);

		DeleteCriticalSection(&m_csLock);
	}
	// 获取一个新的IO_DATA
	LPPER_IO_CONTEXT GetNewIOContext()
	{
		LPPER_IO_CONTEXT context = ioContextPool.AllocateIoContext();
		if (context != NULL)
		{
			EnterCriticalSection(&m_csLock);
			arrIoContext.push_back(context);
			LeaveCriticalSection(&m_csLock);
		}
		return context;
	}
	// 从数组中移除一个指定的IoContext
	void RemoveIOContext(LPPER_IO_CONTEXT ioContext)
	{
		for (std::vector<LPPER_IO_CONTEXT>::iterator it = arrIoContext.begin(); it != arrIoContext.end(); it++)
		{
			if (ioContext == *it)
			{
				ioContextPool.ReleaseIOContext(*it);

				EnterCriticalSection(&m_csLock);
				arrIoContext.erase(it);
				LeaveCriticalSection(&m_csLock);
				break;
			}
		}
	}
	static void ShowIoContextPoolInfo()
	{
		ioContextPool.ShowIOContextPoolInfo();
	}
} PER_SOCKET_CONTEXT, *LPPER_SOCKET_CONTEXT;