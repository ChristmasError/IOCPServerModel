#include<IOCPModel.h>
#include <mstcpip.h>
#define _WINSOCK_DEPRECATED_NO_WARNINGS 
#pragma warning(disable : 4996) 
IOContextPool _PER_HANDLE_DATA::ioContextPool;		// 初始化

/////////////////////////////////////////////////////////////////
// 启动服务器
// public:
bool IOCPModel::ServerStart(bool serveroption)
{
	// 服务器运行状态检测
	if (m_ServerRunning) 
	{
		_ShowMessage("服务器运行中,请勿重复运行！\n");
		return false;
	}
	// 根据创建选择，初始化服务器所需资源
	m_useAcceptEx = serveroption;
	InitializeIOCPResource(m_useAcceptEx);

	// 服务器开始工作
	if (true == m_useAcceptEx)
	{
		_StartEX();
	}
	else// (false == m_useAcceptEx)
	{
		_Start();
	}
	return true;
}
// private:
bool IOCPModel::_StartEX()
{
	m_ServerRunning = RUNNING;
	this->_ShowMessage("本服务器已准备就绪，正在等待客户端的接入......\n");

	while (m_ServerRunning)
	{
		Sleep(1000);
	}
	return true;
}
bool IOCPModel::_Start()
{
	m_ServerRunning = RUNNING;
	this->_ShowMessage("本服务器已准备就绪，正在等待客户端的接入......\n");

	// 服务器accept()接受客户端新套接字
	WinSocket acceptSock; 
	LPPER_HANDLE_DATA handleInfo;

	DWORD RecvBytes, Flags = 0;

	while (true)
	{
		acceptSock = m_ServerSock.Accept();
		acceptSock.SetBlock(false);
		if (INVALID_SOCKET == acceptSock.socket)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
				continue;
			else
			{
				cerr << "Accept Socket Error: " << GetLastError() << endl;
				system("pause");
				return -1;
			}
		}
		handleInfo = new PER_HANDLE_DATA();
		handleInfo->m_Sock = acceptSock;
		CreateIoCompletionPort((HANDLE)(handleInfo->m_Sock.socket), m_IOCompletionPort, (DWORD)handleInfo, 0);
		// 开始在接受套接字上处理I/O使用重叠I/O机制,在新建的套接字上投递一个或多个异步,WSARecv或WSASend请求
		// 这些I/O请求完成后，工作者线程会为I/O请求提供服务	
		// 单I/O操作数据(I/O重叠)
		PER_IO_DATA* ioInfo = new PER_IO_DATA();
		if (!_PostRecv(handleInfo, ioInfo))
		{
			return false;
		}
	}
	return true;
}

/////////////////////////////////////////////////////////////////
// 工作线程函数
DWORD WINAPI IOCPModel::_WorkerThread(LPVOID lpParam)
{
	IOCPModel* IOCP = (IOCPModel*)lpParam;
;
	LPPER_HANDLE_DATA handleInfo = NULL;
	LPPER_IO_DATA ioInfo = NULL;
	DWORD RecvBytes;
	DWORD Flags = 0;

	while (WAIT_OBJECT_0 != WaitForSingleObject(IOCP->m_WorkerShutdownEvent,0))
	{
		bool bRet = GetQueuedCompletionStatus(IOCP->m_IOCompletionPort, &RecvBytes, (PULONG_PTR)&(handleInfo), (LPOVERLAPPED*)&ioInfo, INFINITE);
		//收到退出线程标志，直接退出工作线程

		if (EXIT_CODE == (DWORD)handleInfo)
		{
			break;
		}
		// NULL_POSTED 操作未初始化
		if (ioInfo->m_OpType == NULL_POSTED) 
		{
			continue;
		}
		if(bRet== false)
		{
			DWORD dwError = GetLastError();
			// 是否超时，进入计时等待
			if (dwError == WAIT_TIMEOUT)
			{
				// 客户端仍在活动
				if(/*IsAlive()*/1)
				{
					//ConnectionClose(handleInfo)
						//回收socket
						continue;
				}
				else
				{
					continue;
				}
			}
			// 错误64 客户端异常退出
			else if (dwError == ERROR_NETNAME_DELETED)
			{
				//错误回调函数
				//iocp->ConnectionError				
				//回收socket
			}
			else
			{
				//错误回调函数
				//iocp->ConnectionError				
				//回收socket
			}
		}
		else
		{
			// 判断是否有客户端断开
			if ((RecvBytes == 0) && (ioInfo->m_OpType == RECV_POSTED) ||
				(SEND_POSTED == ioInfo->m_OpType))
			{
				//ConnectionClose(handleInfo);
				//回收socket
				continue;
			}
			else
			{
				switch (ioInfo->m_OpType)
				{
				case ACCEPT_POSTED:
					IOCP->_DoAccept(handleInfo, ioInfo);
					break;
				case RECV_POSTED:
					IOCP->_DoRecv(handleInfo, ioInfo);
					break;
				case SEND_POSTED:
					//IOCP->_DoSend(handleInfo, ioInfo);
					break;
				default:
					break;
				}				
			}
		}
		
	}//while

	// 释放线程参数
	RELEASE_HANDLE(lpParam);
	cout << "工作线程退出！" << endl;
	return 0;
}

/////////////////////////////////////////////////////////////////
// 初始化服务器资源
bool IOCPModel::InitializeIOCPResource(bool useAcceptEX)
{
	// 服务器非运行
	m_ServerRunning = STOP;

	// 初始化退出线程事件
	m_WorkerShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// 初始化IOCP
	if (false == _InitializeIOCP())
	{
		this->_ShowMessage("初始化IOCP失败！\n");
		return false;
	}
	else
	{
		this->_ShowMessage("初始化IOCP完毕！\n");
	}
		
	// 初始化服务器socket,判断是否使用AcceptEX()
	if (useAcceptEX==false)
	{
		if (false == _InitializeServerSocket())
		{
			this->_ShowMessage("初始化服务器Socket失败！\n");
			this->_Deinitialize();
			return false;
		}
		else
		{
			this->_ShowMessage("初始化服务器Socket完毕！\n");
		}
	}
	// 使用AcceptEX()
	if (useAcceptEX == true)
	{
		if (false == _InitializeListenSocket())
		{
			this->_ShowMessage("初始化服务器Socket失败！\n");
			this->_Deinitialize();
			return false;
		}
		else
		{
			this->_ShowMessage("初始化服务器Socket完毕！\n");
		}
	}


	return true;
}

/////////////////////////////////////////////////////////////////
// 加载SOCKET库
// public
bool IOCPModel::LoadSocketLib()
{
	if (_LoadSocketLib() == true)
		return true;
	else
		return false;
}

// 加载SOCKET库
// private
bool IOCPModel::_LoadSocketLib()
{
	int nResult;
	nResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	// 错误(一般都不可能出现)
	if (NO_ERROR != nResult)
	{
		this->_ShowMessage("初始化WinSock 2.2 失败！\n");
		return false;
	}
	return true;
}

/////////////////////////////////////////////////////////////////
// 初始化IOCP:完成端口与工作线程线程池创建
bool IOCPModel::_InitializeIOCP()
{
	// 初始化完成端口
	m_IOCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	//m_WorkThreadShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (NULL == m_IOCompletionPort)
		cout << "创建完成端口失败!\n";

	// 创建IO线程--线程里面创建线程池
	// 基于处理器的核心数量创建线程
	GetSystemInfo(&m_SysInfo);
	for (DWORD i = 0; i < (m_SysInfo.dwNumberOfProcessors * 2); ++i) {
		// 创建服务器工作器线程，并将完成端口传递到该线程
		HANDLE WORKThread = CreateThread(NULL, 0, _WorkerThread, (void*)this, 0, NULL);
		if (NULL == WORKThread) {
			cerr << "创建线程句柄失败！Error:" << GetLastError() << endl;
			system("pause");
			return -1;
		}
		CloseHandle(WORKThread);
	}
	this->_ShowMessage("完成创建工作者线程数量：%d 个\n", m_nThreads);
	return true;	
}


/////////////////////////////////////////////////////////////////
// 初始化服务器Socket
bool IOCPModel::_InitializeListenSocket()
{
	// 创建用于监听的 Listen Socket Context

	m_ListenSockInfo = new PER_HANDLE_DATA;
	m_ListenSockInfo->m_Sock.socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if(INVALID_SOCKET == m_ListenSockInfo->m_Sock.socket)
	{ 
		this->_ShowMessage("创建监听socket context失败!\n");
		return false;
	}
	//listen socket与完成端口绑定
	if (NULL == CreateIoCompletionPort((HANDLE)m_ListenSockInfo->m_Sock.socket, m_IOCompletionPort, (DWORD)m_ListenSockInfo, 0))
	{
		//RELEASE_SOCKET(m_ListenSockInfo->m_Sock.socket)
		this->_ShowMessage("Listen socket与完成端口绑定失败!\n");
		return false;
	}
	// 绑定地址&端口	
	m_ListenSockInfo->m_Sock.port = DEFAULT_PORT;
	m_ListenSockInfo->m_Sock.Bind(m_ListenSockInfo->m_Sock.port);

	// 开始监听
	if (SOCKET_ERROR == listen(m_ListenSockInfo->m_Sock.socket, 5)) {
		cerr << "监听失败. Error: " << GetLastError() << endl;
		return false;
	}


	// AcceptEx 和 GetAcceptExSockaddrs 的GUID，用于导出函数指针
	GUID guidAcceptEx = WSAID_ACCEPTEX;
	GUID guidGetAcceptSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;
	DWORD dwBytes = 0;
	if (WSAIoctl(m_ListenSockInfo->m_Sock.socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidAcceptEx, sizeof(guidAcceptEx), &fnAcceptEx, sizeof(fnAcceptEx),
		&dwBytes, NULL, NULL) == 0)
		cout << "WSAIoctl success..." << endl;
	else {
		cout << "WSAIoctl failed..." << endl;
		switch (WSAGetLastError())
		{
		case WSAENETDOWN:
			cout << "" << endl;
			break;
		case WSAEFAULT:
			cout << "WSAEFAULT" << endl;
			break;
		case WSAEINVAL:
			cout << "WSAEINVAL" << endl;
			break;
		case WSAEINPROGRESS:
			cout << "WSAEINPROGRESS" << endl;
			break;
		case WSAENOTSOCK:
			cout << "WSAENOTSOCK" << endl;
			break;
		case WSAEOPNOTSUPP:
			cout << "WSAEOPNOTSUPP" << endl;
			break;
		case WSA_IO_PENDING:
			cout << "WSA_IO_PENDING" << endl;
			break;
		case WSAEWOULDBLOCK:
			cout << "WSAEWOULDBLOCK" << endl;
			break;
		case WSAENOPROTOOPT:
			cout << "WSAENOPROTOOPT" << endl;
			break;
		}
		return true;
	}

	GUID guidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
	if (0 != WSAIoctl(m_ListenSockInfo->m_Sock.socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidGetAcceptExSockaddrs,
		sizeof(guidGetAcceptExSockaddrs),
		&fnGetAcceptExSockAddrs,
		sizeof(fnGetAcceptExSockAddrs),
		&dwBytes, NULL, NULL))
	{
		//异常
	}

	for (int i = 0; i<200; i++)
	{
		DWORD dwBytes;

		LPPER_IO_DATA  pIO = NULL;
		pIO = new PER_IO_DATA;
		pIO->m_OpType = ACCEPT_POSTED;

		//先创建一个套接字(相比accept有点就在此,accept是接收到连接才创建出来套接字,浪费时间. 这里先准备一个,用于接收连接)
		pIO->m_AcceptSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

		//调用AcceptEx函数，地址长度需要在原有的上面加上16个字节
		//向服务线程投递一个接收连接的的请求
		BOOL rc = fnAcceptEx(m_ListenSockInfo->m_Sock.socket, pIO->m_AcceptSocket,
			pIO->m_buffer, 0,
			sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
			&dwBytes, &(pIO->m_Overlapped));

		if (FALSE == rc)
		{
			if (WSAGetLastError() != ERROR_IO_PENDING)
			{
				printf("%d", WSAGetLastError());
				return false;
			}
		}
	}

	return true;
}
bool IOCPModel::_InitializeServerSocket()
{
	m_ServerSock.CreateSocket();
	m_ServerSock.port = DEFAULT_PORT;
	m_ServerSock.Bind(m_ServerSock.port);

	// 开始监听
	if (SOCKET_ERROR == listen(m_ServerSock.socket, 5)) {
		cerr << "监听失败. Error: " << GetLastError() << endl;
		return false;
	}

	return true;
}


/////////////////////////////////////////////////////////////////
// 释放所有资源
void IOCPModel::_Deinitialize()
{
	// 关闭系统退出事件句柄
	RELEASE_HANDLE(m_WorkerShutdownEvent);

	// 释放工作者线程句柄指针
	for (int i = 0; i < m_nThreads; i++)
	{
		RELEASE_HANDLE(m_WorkerThreadsHandleArray[i]);
	}
	delete[] m_WorkerThreadsHandleArray;

	// 关闭IOCP句柄
	RELEASE_HANDLE(m_IOCompletionPort);

	// 关闭服务器socket
	// ！！！！！还没写 释放 WinSocket    m_ServerSocket;

	this->_ShowMessage("释放资源完毕！\n");
}

/////////////////////////////////////////////////////////////////
// 打印消息
void IOCPModel::_ShowMessage(const char* msg, ...) const
{
	std::cout << msg;
}

/////////////////////////////////////////////////////////////////
// 处理I/O请求
bool IOCPModel::_DoAccept(PER_HANDLE_DATA* handleInfo, PER_IO_DATA *ioInfo)
{
	//InterlockedIncrement(&connectCnt);
	//InterlockedDecrement(&acceptPostCnt);
	SOCKADDR_IN *clientAddr = NULL;
	SOCKADDR_IN *localAddr = NULL;
	int clientAddrLen = sizeof(SOCKADDR_IN);
	int localAddrLen = sizeof(SOCKADDR_IN);

	// 1. 获取地址信息 （GetAcceptExSockAddrs函数不仅可以获取地址信息，还可以顺便取出第一组数据）
	fnGetAcceptExSockAddrs(ioInfo->m_wsaBuf.buf, 0, localAddrLen, clientAddrLen, (LPSOCKADDR *)&localAddr, &localAddrLen, (LPSOCKADDR *)&clientAddr, &clientAddrLen);

	// 2. 为新连接建立一个SocketContext 
	PER_HANDLE_DATA *newSockContext = new PER_HANDLE_DATA;
	newSockContext->m_Sock.socket = ioInfo->m_AcceptSocket;
	//memcpy_s(&(newSockContext->clientAddr), sizeof(SOCKADDR_IN), clientAddr, sizeof(SOCKADDR_IN));

	// 3. 将listenSocketContext的IOContext 重置后继续投递AcceptEx
	ioInfo->Reset();
	if (false == _PostAccept(m_ListenSockInfo, ioInfo))
	{
		m_ListenSockInfo->RemoveIOContext(ioInfo);
	}

	// 4. 将新socket和完成端口绑定
	if (NULL == CreateIoCompletionPort((HANDLE)newSockContext->m_Sock.socket, m_IOCompletionPort, (DWORD)newSockContext, 0))
	{
		DWORD dwErr = WSAGetLastError();
		if (dwErr != ERROR_INVALID_PARAMETER)
		{
			//DoClose(newSockContext);
			return false;
		}
	}

	// 并设置tcp_keepalive
	tcp_keepalive alive_in;
	tcp_keepalive alive_out;
	alive_in.onoff = TRUE;
	alive_in.keepalivetime = 1000 * 60;		// 60s  多长时间（ ms ）没有数据就开始 send 心跳包
	alive_in.keepaliveinterval = 1000 * 10; //10s  每隔多长时间（ ms ） send 一个心跳包
	unsigned long ulBytesReturn = 0;
	if (SOCKET_ERROR == WSAIoctl(newSockContext->m_Sock.socket, SIO_KEEPALIVE_VALS, &alive_in, sizeof(alive_in), &alive_out, sizeof(alive_out), &ulBytesReturn, NULL, NULL))
	{
		//TRACE(L"WSAIoctl failed: %d/n", WSAGetLastError());
	}


	//OnConnectionEstablished(newSockContext);

	// 5. 建立recv操作所需的ioContext，在新连接的socket上投递recv请求
	PER_IO_DATA *newIoContext = newSockContext->GetNewIOContext();
	newIoContext->m_OpType = RECV_POSTED;
	newIoContext->m_AcceptSocket = newSockContext->m_Sock.socket;
	// 投递recv请求
	if (false == _PostRecv(newSockContext, newIoContext))
	{
		//DoClose(sockContext);
		return false;
	}

	return true;
}

bool IOCPModel::_DoRecv(PER_HANDLE_DATA* handleInfo, PER_IO_DATA *ioInfo)
{
	RecvCompleted(handleInfo, ioInfo);

	if (false == _PostRecv(handleInfo, ioInfo))
	{
		this->_ShowMessage("投递WSARecv()失败!\n");
			return false;
	}
	return true;
}

bool IOCPModel::_DoSend(PER_HANDLE_DATA* phd, PER_IO_DATA *pid)
{
	return true;
}


/////////////////////////////////////////////////////////////////
// 投递I/O请求
bool IOCPModel::_PostAccept(PER_HANDLE_DATA* handleInfo, PER_IO_DATA *ioInfo)
{

	//在使用AcceptEx前需要事先重建一个套接字用于其第二个参数。这样目的是节省时间
	//通常可以创建一个套接字库
	ioInfo->m_AcceptSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);

	DWORD flags = 0;
	DWORD dwBytes = 0;
	//调用AcceptEx函数，地址长度需要在原有的上面加上16个字节
	//注意这里使用了重叠模型，该函数的完成将在与完成端口关联的工作线程中处理
	cout << "Process AcceptEx function wait for client connect..." << endl;
	int rc = fnAcceptEx(m_ListenSockInfo->m_Sock.socket, ioInfo->m_AcceptSocket,ioInfo->m_buffer,
		0,
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &dwBytes,
		&(ioInfo->m_Overlapped));
	if (rc == FALSE)
	{
		if (WSAGetLastError() != ERROR_IO_PENDING)
			cout << "lpfnAcceptEx failed.." << endl;
		return false;
	}

	//DWORD dwBytes = 0;
	//ioInfo->m_OpType = ACCEPT_POSTED;
	////ioInfo->m_AcceptSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	//if (INVALID_SOCKET == ioInfo->m_AcceptSocket)
	//{
	//	this->_ShowMessage("_PostAccept() error: 1 !\n");
	//	return false;
	//}

	//// 将接收缓冲置为0,令AcceptEx直接返回,防止拒绝服务攻击
	//if (false == fnAcceptEx(m_ListenSockInfo->m_Sock.socket, ioInfo->m_AcceptSocket, ioInfo->m_wsaBuf.buf, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, &dwBytes, &ioInfo->m_Overlapped))
	//{
	//	if (WSA_IO_PENDING != WSAGetLastError())
	//	{
	//		this->_ShowMessage("_PostAccept() error: 2 !\n");
	//		return false;
	//	}
	//}

	////InterlockedIncrement(&acceptPostCnt);
	return true;
}

bool IOCPModel::_PostRecv(PER_HANDLE_DATA* phd, PER_IO_DATA *pid)
{
	DWORD RecvBytes = 0, Flags = 0;

	pid->Reset();
	pid->m_OpType = RECV_POSTED;	// read
	int nBytesRecv = WSARecv(phd->m_Sock.socket, &(pid->m_wsaBuf), 1, &RecvBytes, &Flags, &(pid->m_Overlapped), NULL);
	if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()))
	{
		cout << "如果返回值错误，并且错误的代码并非是Pending的话，那就说明这个重叠请求失败了\n";
		return false;
	}
	return true;
}

bool IOCPModel::_PostSend(PER_HANDLE_DATA* phd, PER_IO_DATA *pid)
{
	return true;
}