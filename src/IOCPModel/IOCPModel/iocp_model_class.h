#pragma once

#include "winsock_class.h"		
#include "per_io_context_struct.h"
#include "per_socket_context_struct.h"
#include "socket_context_pool_class.h"
#include "cs_auto_lock_class.h"

#include<MSWSock.h>


// 服务器运行状态
#define RUNNING true
#define STOP    false

// 释放句柄
inline void RELEASE_HANDLE(HANDLE handle)
{
	if (handle != NULL && handle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(handle);
		handle = INVALID_HANDLE_VALUE;
	}
}
// 释放指针
inline void RELEASE(void* point)
{
	if (point != NULL)
	{
		delete point;
		point = NULL;
	}
}

//====================================================================================
//
//								IOCPModel类定义
//
//====================================================================================
class IOCPModel
{
public:
	// 服务器内资源初始化
	IOCPModel();
	~IOCPModel();

	// 开启、关闭服务器
	bool StartServer();
	void StopServer();

	// 打印本地(服务器)ip地址
	const char* GetLocalIP();

	// 投递Send请求
	bool PostSend(LPPER_SOCKET_CONTEXT SocketInfo, LPPER_IO_CONTEXT IoInfo)
	{
		return _PostSend(SocketInfo, IoInfo);
	}
	
	// 关闭连接
	void DoClose(LPPER_SOCKET_CONTEXT socketInfo)
	{
		_DoClose(socketInfo);
	}

	// 当前服务器活跃链接数量
	static unsigned int GetConnectCnt()
	{
		return m_ServerSocketPool.NumOfConnectingServer();
	}

	// 事件通知函数(派生类重载此族函数)
	virtual void ConnectionEstablished(LPPER_SOCKET_CONTEXT socketInfo) = 0;
	virtual void ConnectionClosed(LPPER_SOCKET_CONTEXT socketInfo) = 0;
	virtual void ConnectionError(LPPER_SOCKET_CONTEXT socketInfo, DWORD errorNum) = 0;
	virtual void RecvCompleted(LPPER_SOCKET_CONTEXT socketInfo, LPPER_IO_CONTEXT ioInfo) = 0;
	virtual void SendCompleted(LPPER_SOCKET_CONTEXT socketInfo, LPPER_IO_CONTEXT ioInfo) = 0;

private:
	// 初始化服务器资源
	// 1.初始化Winsock服务
	// 2.初始化IOCP + 工作函数线程池
	bool _InitializeServerResource();

	// 1.加载Winsock库:private
	bool _LoadSocketLib();

	// 2.初始化IOCP:完成端口与工作线程线程池
	bool _InitializeIOCP();

	// 释放服务器资源
	void _Deinitialize();

	// 卸载Socket库
	bool _UnloadSocketLib()
	{
		WSACleanup();
	}

	// 建立监听服务器套接字
	bool _InitializeListenSocket();

	// socket是否存活
	bool _IsSocketAlive(LPPER_SOCKET_CONTEXT socketInfo);

	// 处理完成端口上的错误
	bool _HandleError(LPPER_SOCKET_CONTEXT socketInfo, const DWORD&dwErr);

	// 断开与客户端连接
	void _DoClose(LPPER_SOCKET_CONTEXT socketInfo);

	// 打印消息
	void _ShowMessage(const char*, ...) const;

	// 获得本机中处理器的数量
	int _GetNumberOfProcessors();

	// 获取本地(服务器)IP
	const char* _GetLocalIP();

	// 线程函数，为IOCP请求服务工作者线程
	static DWORD WINAPI _WorkerThread(LPVOID lpParam);

	// 处理I/O请求
	bool _DoAccept(LPPER_IO_CONTEXT socketInfo);
	bool _DoRecv(LPPER_SOCKET_CONTEXT socketInfo, LPPER_IO_CONTEXT ioInfo);
	bool _DoSend(LPPER_SOCKET_CONTEXT socketInfo, LPPER_IO_CONTEXT ioInfo);

	// 投递I/O请求
	bool _PostAccept(LPPER_IO_CONTEXT socketInfo);
	bool _PostRecv(LPPER_SOCKET_CONTEXT socketInfo, LPPER_IO_CONTEXT ioInfo);
	bool _PostSend(LPPER_SOCKET_CONTEXT socketInfo, LPPER_IO_CONTEXT ioInfo);

protected:
	bool						  m_ServerRunning;				// 服务器运行状态判断

	SYSTEM_INFO					  m_SysInfo;					// 操作系统信息

	HANDLE						  m_hIOCompletionPort;			// 完成端口句柄

	HANDLE					      m_hWorkerShutdownEvent;		// 通知线程系统推出事件

	HANDLE						  *m_phWorkerThreadArray;       // 工作线程数组的句柄指针

	LPPER_SOCKET_CONTEXT          m_lpListenSockInfo;			// 服务器ListenContext

	LPFN_ACCEPTEX				  m_lpfnAcceptEx;				// AcceptEx函数指针

	LPFN_GETACCEPTEXSOCKADDRS	  m_lpfnGetAcceptExSockAddrs;	// GetAcceptExSockAddrs()函数指针

	unsigned int				  m_nThreads;				    // 工作线程数量

	static SocketContextPool      m_ServerSocketPool;			// 连入客户端的内存池

	CSLock					      m_csLock;						// 临界区锁
};

