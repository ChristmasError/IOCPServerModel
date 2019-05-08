#pragma once

#include<winsock_class.h>		//封装好的socket类库,项目中需加载其dll位置在首目录的lib文件夹中
#include<http_response_class.h>
#include<per_io_context_struct.cpp>
#include<per_socket_context_struct.cpp>
#include<socket_context_pool_class.h>

#include<MSWSock.h>


// 服务器运行状态
#define RUNNING true
#define STOP    false

// 释放指针
inline void RELEASE_POINT(void* point)
{
	if (point != NULL)
	{
		delete point;
		point = NULL;
	}
}
// 释放句柄
inline void RELEASE_HANDLE(HANDLE handle)
{
	if (handle != NULL && handle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(handle);
		handle = NULL;
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

	// 事件通知函数(派生类重载此族函数)
	//virtual void ConnectionEstablished(PER_HANDLE_DATA *handleInfo) = 0;
	//virtual void ConnectionClosed(PER_HANDLE_DATA *handleInfo) = 0;
	//virtual void ConnectionError(PER_HANDLE_DATA *handleInfo, int error) = 0;
	virtual void RecvCompleted(PER_SOCKET_CONTEXT *handleInfo, LPPER_IO_CONTEXT ioInfo) = 0;
	virtual void SendCompleted(PER_SOCKET_CONTEXT *handleInfo, LPPER_IO_CONTEXT ioInfo) = 0;

	bool PostSend(LPPER_SOCKET_CONTEXT SocketInfo, LPPER_IO_CONTEXT IoInfo)
	{
		return _PostSend( SocketInfo, IoInfo );
	}

	static unsigned int NumOfConnectingServer()
	{
		return m_ServerSocketPool.NumOfConnectingServer();
	}
private:
	// 开启服务器
	bool _Start();

	// 初始化服务器资源
	// 1.初始化Winsock服务
	// 2.初始化IOCP + 工作函数线程池
	bool _InitializeServerResource();

	// 释放服务器资源
	void _Deinitialize();

	// 加载Winsock库:private
	bool _LoadSocketLib();

	// 卸载Socket库
	bool _UnloadSocketLib()
	{
		WSACleanup();
	}

	// 初始化IOCP:完成端口与工作线程线程池创建
	bool _InitializeIOCP();

	// 根据服务器工作模式，建立监听服务器套接字
	bool _InitializeListenSocket();

	// 线程函数，为IOCP请求服务工作者线程
	static DWORD WINAPI _WorkerThread(LPVOID lpParam);

	// 打印消息
	void _ShowMessage(const char*, ...) const;

	// 获得本机中处理器的数量
	int _GetNumberOfProcessors();

	// 获取本地(服务器)IP
	const char* _GetLocalIP();

	// 处理I/O请求
	bool _DoAccept(LPPER_IO_CONTEXT pid);
	bool _DoRecv(LPPER_SOCKET_CONTEXT phd, LPPER_IO_CONTEXT pid);
	bool _DoSend(LPPER_SOCKET_CONTEXT phd, LPPER_IO_CONTEXT pid);

	// 投递I/O请求
	bool _PostAccept(LPPER_IO_CONTEXT pid);
	bool _PostRecv(LPPER_SOCKET_CONTEXT phd, LPPER_IO_CONTEXT pid);
	bool _PostSend(LPPER_SOCKET_CONTEXT phd, LPPER_IO_CONTEXT pid);

protected:															
	bool						  m_ServerRunning;				// 服务器运行状态判断

	SYSTEM_INFO					  m_SysInfo;					// 操作系统信息

	HANDLE						  m_hIOCompletionPort;			// 完成端口句柄

	HANDLE					      m_hWorkerShutdownEvent;		// 通知线程系统推出事件

	HANDLE						  *m_phWorkerThreadArray;       // 工作线程的句柄指针

	LPPER_SOCKET_CONTEXT          m_ListenSockInfo;				// 服务器ListenContext

	LPFN_ACCEPTEX				  m_lpfnAcceptEx;				// AcceptEx函数指针
	LPFN_GETACCEPTEXSOCKADDRS	  m_lpfnGetAcceptExSockAddrs;	// GetAcceptExSockAddrs()函数指针

	int							  m_nThreads;				    // 工作线程数量

	static SocketContextPool      m_ServerSocketPool;			// 连入客户端的内存池
};

