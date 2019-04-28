#pragma once

#include<HttpResponse.h>
#include<WinSock.h>		//封装好的socket类库,项目中需加载其dll位置在首目录的lib文件夹中

#include<Windows.h>
#include<MSWSock.h>
#include<vector>
#include<list>
#include<thread>
#include<iostream>

// 服务器运行状态
#define RUNNING true
#define STOP    false

// DATABUF默认大小
#define BUF_SIZE 102400
// 传递给Worker线程的退出信号
#define WORK_THREADS_EXIT NULL
// 同时投递的Accept数量
#define MAX_POST_ACCEPT (2000)
// IOContextPool中的初始数量
#define INIT_IOCONTEXT_NUM (100)				
// 默认端口
#define DEFAULT_PORT 8888
// 退出标志
#define EXIT_CODE (-1)

// 释放句柄
inline void RELEASE_HANDLE(HANDLE handle)
{
	if ( handle != NULL && handle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(handle);
		handle = NULL;
	}
}
// 释放指针
inline void RELEASE_POINT(void* point)
{
	if (point != NULL)
	{
		delete point;
		point = NULL;
	}
}

//========================================================
// 在完成端口上投递的I/O操作的类型
//========================================================
typedef enum _OPERATION_TYPE
{
	ACCEPT_POSTED,		// 标志投递的是接收操作
	RECV_POSTED,		// 标志投递的是接收操作
	SEND_POSTED,		// 标志投递的是发送操作
	NULL_POSTED			// 初始化用

} OPERATION_TYPE;

//====================================================================================
//
//				单IO数据结构体定义(用于每一个重叠操作的参数)
//
//====================================================================================
typedef struct _PER_IO_CONTEXT
{
	WSAOVERLAPPED	m_Overlapped;					// OVERLAPPED结构，该结构里边有一个event事件对象,必须放在结构体首位，作为首地址
	SOCKET			m_AcceptSocket;					// 此IO操作对应的socket
	WSABUF			m_wsaBuf;						// WSABUF结构，包含成员：一个指针指向buf，和一个buf的长度len
	char			m_buffer[BUF_SIZE];			    // WSABUF具体字符缓冲区
	OPERATION_TYPE  m_OpType;						// 标志位

	// 初始化
	_PER_IO_CONTEXT()
	{
		ZeroMemory(&(m_Overlapped), sizeof(WSAOVERLAPPED));
		ZeroMemory(m_buffer, BUF_SIZE);
		m_AcceptSocket = INVALID_SOCKET;
		m_wsaBuf.buf = m_buffer;
		m_wsaBuf.len = BUF_SIZE;
		m_OpType = NULL_POSTED;
	}
	// 释放
	~_PER_IO_CONTEXT()
	{

	}
	// 重置
	void Reset()
	{
		ZeroMemory(&m_Overlapped, sizeof(WSAOVERLAPPED));
		if (m_wsaBuf.buf != NULL)
			ZeroMemory(m_wsaBuf.buf, BUF_SIZE);
		else
			m_wsaBuf.buf= (char *)::HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BUF_SIZE);
		m_OpType = NULL_POSTED;
		ZeroMemory(m_buffer, BUF_SIZE);
	}

} PER_IO_CONTEXT, *LPPER_IO_CONTEXT;
//========================================================
// IOContextPool
//========================================================
class IOContextPool
{
private:
	list<LPPER_IO_CONTEXT> contextList;
	CRITICAL_SECTION csLock;

public:
	IOContextPool()
	{
		InitializeCriticalSection(&csLock);
		contextList.clear();

		EnterCriticalSection(&csLock);
		for (size_t i = 0; i < INIT_IOCONTEXT_NUM; i++)
		{
			LPPER_IO_CONTEXT context = new PER_IO_CONTEXT;
			contextList.push_back(context);
		}
		LeaveCriticalSection(&csLock);

	}

	~IOContextPool()
	{
		EnterCriticalSection(&csLock);
		for (list<LPPER_IO_CONTEXT>::iterator it = contextList.begin(); it != contextList.end(); it++)
		{
			delete (*it);
		}
		contextList.clear();
		LeaveCriticalSection(&csLock);

		DeleteCriticalSection(&csLock);
	}

	// 分配一个IOContxt
	LPPER_IO_CONTEXT AllocateIoContext()
	{
		LPPER_IO_CONTEXT context = NULL;

		EnterCriticalSection(&csLock);
		if (contextList.size() > 0) //list不为空，从list中取一个
		{
			context = contextList.back();
			contextList.pop_back();
		}
		else	//list为空，新建一个
		{
			context = new PER_IO_CONTEXT;
		}
		LeaveCriticalSection(&csLock);

		return context;
	}

	// 回收一个IOContxt
	void ReleaseIOContext(LPPER_IO_CONTEXT pContext)
	{
		pContext->Reset();
		EnterCriticalSection(&csLock);
		contextList.push_front(pContext);
		LeaveCriticalSection(&csLock);
	}
};
//====================================================================================
//
//				单句柄数据结构体定义(用于每一个完成端口，也就是每一个Socket的参数)
//
//====================================================================================
typedef struct _PER_SOCKET_CONTEXT
{	
private:
	vector<LPPER_IO_CONTEXT>	arrIoContext;		  // 同一个socket上的多个IO请求
	static IOContextPool		ioContextPool;		  //空闲的IOcontext池子

public:
	WinSock					m_Sock;		          //每一个socket的信息

	// 初始化
	_PER_SOCKET_CONTEXT()
	{
		m_Sock.socket = INVALID_SOCKET;
	}
	// 释放资源
	~_PER_SOCKET_CONTEXT()
	{
	}
	// 获取一个新的IO_DATA
	LPPER_IO_CONTEXT GetNewIOContext()
	{
		LPPER_IO_CONTEXT context = ioContextPool.AllocateIoContext();
		if (context != NULL)
		{
			//EnterCriticalSection(&csLock);
			arrIoContext.push_back(context);
			//LeaveCriticalSection(&csLock);
		}
		return context;
	}
	// 从数组中移除一个指定的IoContext
	void RemoveIOContext(LPPER_IO_CONTEXT pContext)
	{
		for (vector<LPPER_IO_CONTEXT>::iterator it = arrIoContext.begin(); it != arrIoContext.end(); it++)
		{
			if (pContext == *it)
			{
				ioContextPool.ReleaseIOContext(*it);

				//EnterCriticalSection(&csLock);
				arrIoContext.erase(it);
				//LeaveCriticalSection(&csLock);

				break;
			}
		}
	}

} PER_SOCKET_CONTEXT,*LPPER_SOCKET_CONTEXT;

//====================================================================================
//
//								IOCPModel类定义
//
//====================================================================================
class IOCPModel
{
public:
	// 服务器内资源初始化
	IOCPModel():m_ServerRunning(STOP),
				m_hIOCompletionPort(INVALID_HANDLE_VALUE),
				m_phWorkerThreadArray(NULL),
				m_ListenSockInfo(NULL),
				m_lpfnAcceptEx(NULL),
				m_lpfnGetAcceptExSockAddrs(NULL),
				m_nThreads(0)
	{
		if (_LoadSocketLib() == true)
			this->_ShowMessage("加载Winsock库成功！\n");
		else
			// 加载失败 抛出异常
		// 初始化退出线程事件
		m_hWorkerShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	}
	~IOCPModel()
	{
		_Deinitialize();
	}
	// 开启、关闭服务器
	bool StartServer();
	void StopServer();	

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

	PER_SOCKET_CONTEXT            *m_ListenSockInfo;			// 服务器ListenContext

	LPFN_ACCEPTEX				  m_lpfnAcceptEx;				// AcceptEx函数指针
	LPFN_GETACCEPTEXSOCKADDRS	  m_lpfnGetAcceptExSockAddrs;	// GetAcceptExSockAddrs()函数指针

	int							  m_nThreads;				    // 工作线程数量
};
