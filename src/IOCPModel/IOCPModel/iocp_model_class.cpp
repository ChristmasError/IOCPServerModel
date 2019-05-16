#pragma once

#include "iocp_model_class.h"
#include <mstcpip.h>

// 传递给Worker线程的退出信号
#define WORK_THREADS_EXIT NULL
// 同时投递的Accept数量
#define MAX_POST_ACCEPT (500)				
// 默认端口
#define DEFAULT_PORT 8888
// 退出标志
#define EXIT_CODE (-1)

IOContextPool _PER_SOCKET_CONTEXT::ioContextPool;
SocketContextPool IOCPModel::m_ServerSocketPool;

//=========================================================================
//
//							初始化与释放
//
//=========================================================================

// 构造
IOCPModel::IOCPModel() :
	m_ServerRunning(STOP),
	m_hIOCompletionPort(INVALID_HANDLE_VALUE),
	m_phWorkerThreadArray(NULL),
	m_ListenSockInfo(NULL),
	m_lpfnAcceptEx(NULL),
	m_lpfnGetAcceptExSockAddrs(NULL),
	m_nThreads(0)
{
	if (_LoadSocketLib() == true)
		this->_ShowMessage("初始化WinSock 2.2成功...\n");
	else
		// 加载失败 抛出异常
		// 初始化退出线程事件
		m_hWorkerShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}
// 析构
IOCPModel::~IOCPModel()
{
	_Deinitialize();
}
/////////////////////////////////////////////////////////////////
// 初始化资源,启动服务器相关
bool IOCPModel::StartServer()
{
	// 服务器运行状态检测
	if (m_ServerRunning)
	{
		this->_ShowMessage("服务器运行中,请勿重复运行!\n");
		return false;
	}
	// 初始化服务器所需资源
	_InitializeServerResource();

	// 服务器开始工作
	_Start();
}

bool IOCPModel::_Start()
{
	m_ServerRunning = RUNNING;
	this->_ShowMessage("服务器已准备就绪:IP %s 正在等待客户端的接入......\n", GetLocalIP());

	while (m_ServerRunning)
	{
		Sleep(1000);
	}
	return true;
}
/////////////////////////////////////////////////////////////////
// 初始化服务器资源
bool IOCPModel::_InitializeServerResource()
{
	// 初始化IOCP
	if (false == _InitializeIOCP())
	{
		this->_ShowMessage("初始化IOCP失败!\n");
		return false;
	}
	else
	{
		this->_ShowMessage("初始化IOCP完毕...\n");
	}

	// 初始化服务器ListenSocket
	if (false == _InitializeListenSocket())
	{
		this->_ShowMessage("初始化服务器ListenSocket失败!\n");
		this->_Deinitialize();
		return false;
	}
	else
	{
		this->_ShowMessage("初始化服务器ListenSocket完毕...\n");
	}
	return true;
}

/////////////////////////////////////////////////////////////////
// 加载SOCKET库
bool IOCPModel::_LoadSocketLib()
{
	WSADATA	wsaData;	// Winsock服务初始化
	int nResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	// 错误(一般都不可能出现)
	if (NO_ERROR != nResult)
	{
		this->_ShowMessage("_LoadSocketLib()：初始化WinSock 2.2 失败!\n");
		return false;
	}
	return true;
}

/////////////////////////////////////////////////////////////////
// 初始化IOCP:完成端口与工作线程线程池创建
bool IOCPModel::_InitializeIOCP()
{
	// 初始化完成端口
	m_hIOCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (NULL == m_hIOCompletionPort)
		this->_ShowMessage("_InitializeIOCP()：创建完成端口失败!\n");

	// 创建IO线程--线程里面创建线程池
	// 基于处理器的核心数量创建线程
	m_nThreads = _GetNumberOfProcessors() * 2;
	m_phWorkerThreadArray = new HANDLE[m_nThreads];
	for (DWORD i = 0; i <m_nThreads; ++i)
	{
		// 创建服务器工作器线程，并将完成端口传递到该线程
		HANDLE hWorkThread = CreateThread(NULL, NULL, _WorkerThread, (void*)this, 0, NULL);
		if (NULL == hWorkThread) {
			this->_ShowMessage("_InitializeIOCP()：创建线程句柄失败! Error:%d\n", GetLastError());
			system("pause");
			return false;
		}
		m_phWorkerThreadArray[i] = hWorkThread;
		CloseHandle(hWorkThread);
	}
	this->_ShowMessage("_InitializeIOCP()：完成创建工作者线程数量：%d 个...\n", m_nThreads);
	return true;
}

/////////////////////////////////////////////////////////////////
// 初始化listenSocket
bool IOCPModel::_InitializeListenSocket()
{
	// 创建用于监听的 Listen Socket Context

	m_ListenSockInfo = new PER_SOCKET_CONTEXT;
	m_ListenSockInfo->m_Sock.CreateWSASocket();
	if (INVALID_SOCKET == m_ListenSockInfo->m_Sock.socket)
	{
		this->_ShowMessage("_InitializeListenSocket()：创建监听socket失败!\n");
		return false;
	}
	//listen socket与完成端口绑定
	if (NULL == CreateIoCompletionPort((HANDLE)m_ListenSockInfo->m_Sock.socket, m_hIOCompletionPort, (DWORD)m_ListenSockInfo, 0))
	{
		//RELEASE_SOCKET(m_ListenSockInfo->m_Sock.socket)
		this->_ShowMessage("_InitializeListenSocket()：监听socket与完成端口绑定失败!\n");
		return false;
	}
	else
	{
		this->_ShowMessage("_InitializeListenSocket()：监听socket与完成端口绑定完成...\n");
	}
	// 绑定地址&端口	
	m_ListenSockInfo->m_Sock.Bind(DEFAULT_PORT);

	// 开始进行监听
	if (SOCKET_ERROR == listen(m_ListenSockInfo->m_Sock.socket, SOMAXCONN)) {
		this->_ShowMessage("_InitializeListenSocket()：监听失败. Error:%d\n", GetLastError());
		return false;
	}
	else
	{
		this->_ShowMessage("_InitializeListenSocket()：监听开始...\n");
	}

	// AcceptEx 和 GetAcceptExSockaddrs 的GUID，用于导出函数指针
	GUID guidAcceptEx = WSAID_ACCEPTEX;
	GUID guidGetAcceptEXSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;
	DWORD dwBytes = 0;
	if (SOCKET_ERROR == WSAIoctl(
		m_ListenSockInfo->m_Sock.socket,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidAcceptEx,
		sizeof(guidAcceptEx),
		&m_lpfnAcceptEx,
		sizeof(m_lpfnAcceptEx),
		&dwBytes,
		NULL,
		NULL))
	{
		this->_ShowMessage("WSAIoctl()未能获取AcceptEx函数指针!错误代码: %d\n", WSAGetLastError());
		// 释放资源_Deinitialize
		return false;
	}

	// 同理，获取GetAcceptExSockAddrs函数指针
	if (SOCKET_ERROR == WSAIoctl(
		m_ListenSockInfo->m_Sock.socket,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidGetAcceptEXSockAddrs,
		sizeof(guidGetAcceptEXSockAddrs),
		&m_lpfnGetAcceptExSockAddrs,
		sizeof(m_lpfnGetAcceptExSockAddrs),
		&dwBytes,
		NULL,
		NULL))
	{
		this->_ShowMessage("WSAIoctl 未能获取GetAcceptExSockAddrs函数指针,错误代码: %d\n", WSAGetLastError());
		// 释放资源_Deinitialize
		return false;
	}

	for (int i = 0; i < MAX_POST_ACCEPT; i++)
	{
		// 投递accept
		LPPER_IO_CONTEXT  pAcceptIoContext = new PER_IO_CONTEXT;
		if (false == this->_PostAccept(pAcceptIoContext))
		{
			//m_pListenContext->RemoveContext(pAcceptIoContext);
			return false;
		}
	}
	this->_ShowMessage("投递 %d 个AcceptEx请求完毕...\n", MAX_POST_ACCEPT);
	return true;
}

/////////////////////////////////////////////////////////////////
// 释放所有资源
void IOCPModel::_Deinitialize()
{
	// 关闭系统退出事件句柄
	RELEASE_HANDLE(m_hWorkerShutdownEvent);

	// 释放工作者线程句柄指针
	for (int i = 0; i < m_nThreads; i++)
	{
		RELEASE_HANDLE(m_phWorkerThreadArray[i]);
	}
	delete[] m_phWorkerThreadArray;

	// 关闭IOCP句柄
	RELEASE_HANDLE(m_hIOCompletionPort);

	this->_ShowMessage("服务器释放资源完毕...\n");
}

/////////////////////////////////////////////////////////////////
// 关闭服务器
void IOCPModel::StopServer()
{
	SetEvent(m_hWorkerShutdownEvent);
	for (int i = 0; i < m_nThreads; i++)
	{
		PostQueuedCompletionStatus(m_hIOCompletionPort, 0, (DWORD)EXIT_CODE, NULL);
	}
	WaitForMultipleObjects(m_nThreads, m_phWorkerThreadArray, TRUE, INFINITE);
	// 释放资源
	_Deinitialize();
}

//=========================================================================
//
//							工作线程函数
//
//=========================================================================
DWORD WINAPI IOCPModel::_WorkerThread(LPVOID lpParam)
{
	IOCPModel* IOCP = (IOCPModel*)lpParam;
	;
	LPPER_SOCKET_CONTEXT socketInfo = NULL;
	LPPER_IO_CONTEXT ioInfo = NULL;
	DWORD RecvBytes = 0;
	DWORD Flags = 0;

	while (WAIT_OBJECT_0 != WaitForSingleObject(IOCP->m_hWorkerShutdownEvent, 0))
	{
		bool bRet = GetQueuedCompletionStatus(IOCP->m_hIOCompletionPort, &RecvBytes, (PULONG_PTR)&(socketInfo), (LPOVERLAPPED*)&ioInfo, INFINITE);
		// 收到EXIT_CODE退出线程标志，退出工作线程
		if (EXIT_CODE == (DWORD)socketInfo)
		{
			break;
		}
		// NULL_POSTED 操作未初始化
		if (ioInfo->m_OpType == NULL_POSTED)
		{
			continue;
		}
		if (bRet == false)
		{
			DWORD dwError = GetLastError();
			// 超时检测，进入计时等待
			if (WAIT_TIMEOUT == dwError)
			{
				// 客户端仍在活动
				if (IOCP->_IsSocketAlive(socketInfo))
				{
					IOCP->ConnectionClosed(socketInfo);
					//ConnectionClose(handleInfo)
					continue;
				}
				else
				{
					continue;
				}
			}
			// 错误64 客户端异常退出
			else if (ERROR_NETNAME_DELETED == dwError)
			{
				IOCP->ConnectionError(socketInfo, dwError);
				//ConnectionClose(handleInfo)
			}
			else
			{
				IOCP->ConnectionError(socketInfo, dwError);
				//ConnectionClose(handleInfo)
			}
		}
		else
		{
			// 心跳包判断是否有客户端断开
			if ((RecvBytes == 0) && (ioInfo->m_OpType == RECV_POSTED))
			{
				IOCP->ConnectionClosed(socketInfo);
				//ConnectionClose(handleInfo);
				continue;
			}
			else
			{
				switch (ioInfo->m_OpType)
				{
				case ACCEPT_POSTED:
					IOCP->_DoAccept(ioInfo);
					break;
				case RECV_POSTED:
					IOCP->_DoRecv(socketInfo, ioInfo);
					break;
				case SEND_POSTED:
					IOCP->_DoSend(socketInfo, ioInfo);
					break;
				default:
					break;
				}
			}
		}

	}//while

	 // 释放线程参数
	RELEASE_HANDLE(lpParam);
	IOCP->_ShowMessage("工作线程退出!\n");
	return 0;
}

//=========================================================================
//
//							辅助函数定义
//
//=========================================================================

/////////////////////////////////////////////////////////////////
// 获取本地(服务器)IP
const char* IOCPModel::GetLocalIP()
{
	return _GetLocalIP();
}
const char* IOCPModel::_GetLocalIP()
{
	return m_ListenSockInfo->m_Sock.GetLocalIP();
}

///////////////////////////////////////////////////////////////////
// socket是否存活
bool IOCPModel::_IsSocketAlive(LPPER_SOCKET_CONTEXT socketInfo)
{
	int nByteSend = socketInfo->m_Sock.Send("", 0);
	if (nByteSend == -1)
		return false;
	else
		return true;
}

/////////////////////////////////////////////////////////////////
// 断开与客户端连接
bool IOCPModel::_DoClose(LPPER_SOCKET_CONTEXT socektContext)
{
	CSAutoLock cslock(m_csLock);
	delete socektContext;
	return true;
}

///////////////////////////////////////////////////////////////////
// 获得本机中处理器的数量
int IOCPModel::_GetNumberOfProcessors()
{
	GetSystemInfo(&m_SysInfo);
	return m_SysInfo.dwNumberOfProcessors;
}

/////////////////////////////////////////////////////////////////
// 打印消息
void IOCPModel::_ShowMessage(const char* msg, ...) const
{
	
	va_list valist;
	va_start(valist,msg);
	vprintf(msg, valist);
	//std::cout << va_arg(msg, const char*) << std::endl;
	va_end(valist);

}

//=========================================================================
//
//							投递完成端口请求
//
//=========================================================================

/////////////////////////////////////////////////////////////////
// 投递I/O请求
bool IOCPModel::_PostAccept(LPPER_IO_CONTEXT pAcceptioInfo)
{
	ASSERT(ioInfo != m_ListenSockInfo.m_sock.socket);

	//准备参数
	// 与accept的区别,为新连接的客户端准备好socket
	DWORD dwBytes = 0;
	pAcceptioInfo->m_OpType = ACCEPT_POSTED;
	pAcceptioInfo->m_AcceptSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == pAcceptioInfo->m_AcceptSocket)
	{
		this->_ShowMessage("创建用于Accept的Socket失败!错误代码: %d", WSAGetLastError());
		return false;
	}

	// 投递AccpetEX
	if (FALSE == m_lpfnAcceptEx(
		m_ListenSockInfo->m_Sock.socket,
		pAcceptioInfo->m_AcceptSocket,
		pAcceptioInfo->m_wsaBuf.buf,
		0,
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
		&dwBytes,
		&pAcceptioInfo->m_Overlapped))
	{
		if (ERROR_IO_PENDING != WSAGetLastError())
		{
			this->_ShowMessage("投递AcceptEx()请求失败!错误代码: %d", WSAGetLastError());
			return false;
		}
	}
	return true;
}

bool IOCPModel::_PostRecv(LPPER_SOCKET_CONTEXT SocketInfo, LPPER_IO_CONTEXT ioInfo)
{
	ioInfo->Reset();
	ioInfo->m_OpType = RECV_POSTED;
	DWORD RecvBytes = 0, Flags = 0;
	int nBytesRecv = WSARecv(SocketInfo->m_Sock.socket, &(ioInfo->m_wsaBuf), 1, &RecvBytes, &Flags, &(ioInfo->m_Overlapped), NULL);
	if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()))
	{
		this->_ShowMessage( "返回值错误，错误代码非Pending，重叠请求失效!\n");
		return false;
	}
	return true;
}

bool IOCPModel::_PostSend(LPPER_SOCKET_CONTEXT SocketInfo, LPPER_IO_CONTEXT ioInfo)
{
	ioInfo->m_OpType = SEND_POSTED;
	DWORD dwFlags = 0, dwBytes = 0;
	if (::WSASend(ioInfo->m_AcceptSocket, &ioInfo->m_wsaBuf, 1, &dwBytes, dwFlags, &ioInfo->m_Overlapped, NULL) != NO_ERROR)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			//DoClose(SocketInfo);
			return false;
		}
	}
	return true;
}
/////////////////////////////////////////////////////////////////
// 处理I/O请求
bool IOCPModel::_DoAccept(LPPER_IO_CONTEXT ioInfo)
{
	SOCKADDR_IN *clientAddr = NULL;
	SOCKADDR_IN *localAddr = NULL;
	int clientAddrLen = sizeof(SOCKADDR_IN);
	int localAddrLen = sizeof(SOCKADDR_IN);

	// 1. 获取地址信息 （GetAcceptExSockAddrs函数不仅可以获取地址信息，还可以顺便取出第一组数据）
	m_lpfnGetAcceptExSockAddrs(ioInfo->m_wsaBuf.buf, 0, localAddrLen, clientAddrLen, (LPSOCKADDR *)&localAddr, &localAddrLen, (LPSOCKADDR *)&clientAddr, &clientAddrLen);

	// 2. 每一个客户端连入，就为新连接建立一个SocketContext 
	LPPER_SOCKET_CONTEXT pNewSocketInfo = m_ServerSocketPool.AllocateSocketContext();
	pNewSocketInfo->m_Sock.socket = ioInfo->m_AcceptSocket;
	if (INVALID_SOCKET == pNewSocketInfo->m_Sock.socket)
	{
		m_ServerSocketPool.ReleaseSocketContext(pNewSocketInfo);
		this->_ShowMessage("无效的客户端socket!\n");
		return false;
	}
	memcpy(&(pNewSocketInfo->m_Sock.addr), clientAddr, sizeof(SOCKADDR_IN));
	pNewSocketInfo->m_Sock.ip = inet_ntoa(clientAddr->sin_addr);
	// 3. 将新SocketContext和完成端口绑定
	if (NULL == CreateIoCompletionPort((HANDLE)pNewSocketInfo->m_Sock.socket, m_hIOCompletionPort, (DWORD)pNewSocketInfo, 0))
	{
		DWORD dwErr = WSAGetLastError();
		if (dwErr != ERROR_INVALID_PARAMETER)
		{
			//DoClose(newSockContext);
			return false;
		}
	}

	// 4. 为这个客户端建立一个NewIoContext,并投递一个RECV
	LPPER_IO_CONTEXT pNewIoContext = pNewSocketInfo->GetNewIOContext();
	pNewIoContext->m_OpType = RECV_POSTED;
	pNewIoContext->m_AcceptSocket = pNewSocketInfo->m_Sock.socket;
	if (false == this->_PostRecv(pNewSocketInfo, pNewIoContext))
	{
		this->_ShowMessage( "投递WSARecv()失败!\n");
		//DoClose(sockContext);
		return false;
	}

	// 5. 将listenSocketContext的IOContext 重置后继续投递AcceptEx
	ioInfo->Reset();
	if (false == this->_PostAccept(ioInfo))
	{
		m_ListenSockInfo->RemoveIOContext(ioInfo);
	}

	// 设置tcp_keepalive
	tcp_keepalive alive_in;
	tcp_keepalive alive_out;
	alive_in.onoff = TRUE;
	alive_in.keepalivetime = 1000 * 60;		// 60s  多长时间（ ms ）没有数据就开始 send 心跳包
	alive_in.keepaliveinterval = 1000 * 10; //10s  每隔多长时间（ ms ） send 一个心跳包
	unsigned long ulBytesReturn = 0;
	if (SOCKET_ERROR == WSAIoctl(pNewSocketInfo->m_Sock.socket, SIO_KEEPALIVE_VALS, &alive_in, sizeof(alive_in), &alive_out, sizeof(alive_out), &ulBytesReturn, NULL, NULL))
	{
		//TRACE(L"WSAIoctl failed: %d/n", WSAGetLastError());
	}

	this->ConnectionEstablished(pNewSocketInfo);
	return true;
}

bool IOCPModel::_DoRecv(LPPER_SOCKET_CONTEXT SocketInfo, LPPER_IO_CONTEXT ioInfo)
{
	this->RecvCompleted(SocketInfo, ioInfo);
	// 在此socketInfo上投递新的RECV请求
	LPPER_IO_CONTEXT pNewIoContext = SocketInfo->GetNewIOContext();
	pNewIoContext->m_OpType = RECV_POSTED;
	pNewIoContext->m_AcceptSocket = SocketInfo->m_Sock.socket;
	if (false == _PostRecv(SocketInfo, pNewIoContext))
	{
		this->_ShowMessage("投递新WSARecv()失败!\n");
		return false;
	}
	return true;
}

bool IOCPModel::_DoSend(LPPER_SOCKET_CONTEXT SocketInfo, LPPER_IO_CONTEXT ioInfo)
{
	this->SendCompleted(SocketInfo, ioInfo);
	return true;
}

