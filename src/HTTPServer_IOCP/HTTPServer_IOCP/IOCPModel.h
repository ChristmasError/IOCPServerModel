#pragma once

#include<XHttpResponse.h>
#include<WinSock.h>

//#include<WinSocket.h>		//封装好的socket类库,项目中需加载其dll位置在首目录的lib文件夹中
//#include<WinSock2.h>
#include<Windows.h>
#include<MSWSock.h>

#include<vector>
#include<string>
#include<list>

#include<iostream>


#pragma comment(lib, "Kernel32.lib")	// IOCP需要用到的动态链接库

// 服务器运行状态
#define RUNNING true
#define STOP    false
#define EX      true

// DATABUF默认大小
#define BUF_SIZE 1024
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

//========================================================
// 单IO数据结构体定义(用于每一个重叠操作的参数)
//========================================================
typedef struct _PER_IO_DATA
{
	WSAOVERLAPPED	m_Overlapped;					// OVERLAPPED结构，该结构里边有一个event事件对象,必须放在结构体首位，作为首地址
	SOCKET			m_AcceptSocket;					// 此IO操作对应的socket
	WSABUF			m_wsaBuf;						// WSABUF结构，包含成员：一个指针指向buf，和一个buf的长度len
	char			m_buffer[BUF_SIZE];			    // WSABUF具体字符缓冲区
	OPERATION_TYPE  m_OpType;						// 标志位

	// 初始化
	_PER_IO_DATA()
	{
		ZeroMemory(&(m_Overlapped), sizeof(WSAOVERLAPPED));
		ZeroMemory(m_buffer, BUF_SIZE);
		m_AcceptSocket = INVALID_SOCKET;
		m_wsaBuf.buf = m_buffer;
		m_wsaBuf.len = BUF_SIZE;
		m_OpType = NULL_POSTED;
	}
	// 释放
	~_PER_IO_DATA()
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
} PER_IO_DATA, *LPPER_IO_DATA;
//========================================================
// IOContextPool
//========================================================
class IOContextPool
{
private:
	list<PER_IO_DATA *> contextList;
	CRITICAL_SECTION csLock;

public:
	IOContextPool()
	{
		InitializeCriticalSection(&csLock);
		contextList.clear();

		EnterCriticalSection(&csLock);
		for (size_t i = 0; i < INIT_IOCONTEXT_NUM; i++)
		{
			PER_IO_DATA *context = new PER_IO_DATA;
			contextList.push_back(context);
		}
		LeaveCriticalSection(&csLock);

	}

	~IOContextPool()
	{
		EnterCriticalSection(&csLock);
		for (list<PER_IO_DATA *>::iterator it = contextList.begin(); it != contextList.end(); it++)
		{
			delete (*it);
		}
		contextList.clear();
		LeaveCriticalSection(&csLock);

		DeleteCriticalSection(&csLock);
	}

	// 分配一个IOContxt
	PER_IO_DATA *AllocateIoContext()
	{
		PER_IO_DATA *context = NULL;

		EnterCriticalSection(&csLock);
		if (contextList.size() > 0) //list不为空，从list中取一个
		{
			context = contextList.back();
			contextList.pop_back();
		}
		else	//list为空，新建一个
		{
			context = new PER_IO_DATA;
		}
		LeaveCriticalSection(&csLock);

		return context;
	}

	// 回收一个IOContxt
	void ReleaseIOContext(PER_IO_DATA *pContext)
	{
		pContext->Reset();
		EnterCriticalSection(&csLock);
		contextList.push_front(pContext);
		LeaveCriticalSection(&csLock);
	}
};
//========================================================
// 单IO数据结构题定义(每一个客户端socket参数）
//========================================================
typedef struct _PER_HANDLE_DATA
{	
private:
	vector<PER_IO_DATA*>	arrIoContext;		  // 同一个socket上的多个IO请求
	static IOContextPool    ioContextPool;		  //空闲的IOcontext池子
	//CRITICAL_SECTION		csLock;
	//vector<PER_IO_DATA*>    m_vecIoContex;	  //每个socket接收到的所有IO请求数组
public:
	WinSock					m_Sock;		          //每一个socket的信息

	// 初始化
	_PER_HANDLE_DATA()
	{
		m_Sock.socket = INVALID_SOCKET;
	}
	// 释放资源
	~_PER_HANDLE_DATA()
	{
	}

	// 获取一个新的IO_DATA
	PER_IO_DATA *GetNewIOContext()
	{
		PER_IO_DATA *context = ioContextPool.AllocateIoContext();
		if (context != NULL)
		{
			//EnterCriticalSection(&csLock);
			arrIoContext.push_back(context);
			//LeaveCriticalSection(&csLock);
		}
		return context;
	}
	// 从数组中移除一个指定的IoContext
	void RemoveIOContext(PER_IO_DATA* pContext)
	{
		for (vector<PER_IO_DATA*>::iterator it = arrIoContext.begin(); it != arrIoContext.end(); it++)
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
} PER_HANDLE_DATA,*LPPER_HANDLE_DATA;

//========================================================
//					IOCPModel类定义
//========================================================
class IOCPModel
{
public:
	// 服务器内资源初始化
	IOCPModel():m_useAcceptEx(false),
				m_ServerRunning(STOP),
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
	// 开启服务器
	// 根据serveroption判断accept()/acceptEX()工作模式
	bool StartServer(bool serveroption = false);
	
	// 关闭服务器
	void StopServer();	

	// 事件通知函数(派生类重载此族函数)
	//virtual void ConnectionEstablished(PER_HANDLE_DATA *handleInfo) = 0;
	//virtual void ConnectionClosed(PER_HANDLE_DATA *handleInfo) = 0;
	//virtual void ConnectionError(PER_HANDLE_DATA *handleInfo, int error) = 0;
	virtual void RecvCompleted(PER_HANDLE_DATA *handleInfo, PER_IO_DATA *ioInfo) = 0;
	//virtual void SendCompleted(PER_HANDLE_DATA *handleInfo, PER_IO_DATA *ioInfo) = 0;

private:
	// 开启服务器
	bool _Start();
	bool _StartEX();

	// 初始化服务器资源
	// 1.初始化Winsock服务
	// 2.初始化IOCP + 工作函数线程池
	bool _InitializeServerResource(bool useAcceptEX);

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
	bool _InitializeServerSocket();
	bool _InitializeListenSocket();

	// 线程函数，为IOCP请求服务工作者线程
	static DWORD WINAPI _WorkerThread(LPVOID lpParam);

	// 打印消息
	void _ShowMessage(const char*, ...) const;

	// 获得本机中处理器的数量
	int _GetNumberOfProcessors();

	// 处理I/O请求
	bool _DoAccept(PER_HANDLE_DATA* phd, PER_IO_DATA *pid);
	bool _DoRecv(PER_HANDLE_DATA* phd, PER_IO_DATA *pid);
	bool _DoSend(PER_HANDLE_DATA* phd, PER_IO_DATA *pid);

	// 投递I/O请求
	bool _PostAccept(PER_IO_DATA *pid);
	bool _PostRecv(PER_HANDLE_DATA* phd, PER_IO_DATA *pid);
	bool _PostSend(PER_HANDLE_DATA* phd, PER_IO_DATA *pid);

protected:
	bool						  m_useAcceptEx;				// 使用acceptEX()==true || 使用accept()==false
																
	bool						  m_ServerRunning;				// 服务器运行状态判断

	WSADATA						  wsaData;						// Winsock服务初始化

	SYSTEM_INFO					  m_SysInfo;					// 操作系统信息

	HANDLE						  m_hIOCompletionPort;			// 完成端口句柄

	HANDLE					      m_hWorkerShutdownEvent;		// 通知线程系统推出事件

	HANDLE						  *m_phWorkerThreadArray;       // 工作线程的句柄指针

	PER_HANDLE_DATA               *m_ListenSockInfo;			// 服务器监听Context

	LPFN_ACCEPTEX				  m_lpfnAcceptEx;					// AcceptEx函数指针
	LPFN_GETACCEPTEXSOCKADDRS	  m_lpfnGetAcceptExSockAddrs;		// GetAcceptExSockAddrs()函数指针

	WinSock  					  m_ServerSock;				    // 服务器socket信息

	int							  m_nThreads;				    // 工作线程数量
};
