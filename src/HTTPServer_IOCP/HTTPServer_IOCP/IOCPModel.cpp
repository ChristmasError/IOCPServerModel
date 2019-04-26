#include <IOCPModel.h>
#include <mstcpip.h>

IOContextPool _PER_SOCKET_DATA::ioContextPool;		// static成员初始化

/////////////////////////////////////////////////////////////////
// 启动服务器
// public:
bool IOCPModel::StartServer(bool serveroption)
{
	// 服务器运行状态检测
	if (m_ServerRunning) 
	{
		_ShowMessage("服务器运行中,请勿重复运行！\n");
		return false;
	}
	// 根据创建选择，初始化服务器所需资源
	m_useAcceptEx = serveroption;
	_InitializeServerResource(m_useAcceptEx);

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
	WinSock acceptSock; 
	LPPER_SOCKET_DATA handleInfo;

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
		handleInfo = new PER_SOCKET_DATA();
		handleInfo->m_Sock = acceptSock;
		CreateIoCompletionPort((HANDLE)(handleInfo->m_Sock.socket), m_hIOCompletionPort, (DWORD)handleInfo, 0);
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
/////////////////////////////////////////////////////////////////
// 工作线程函数
DWORD WINAPI IOCPModel::_WorkerThread(LPVOID lpParam)
{
	IOCPModel* IOCP = (IOCPModel*)lpParam;
;
	LPPER_SOCKET_DATA handleInfo = NULL;
	LPPER_IO_DATA ioInfo = NULL;
	DWORD RecvBytes;
	DWORD Flags = 0;

	while (WAIT_OBJECT_0 != WaitForSingleObject(IOCP->m_hWorkerShutdownEvent,0))
	{
		bool bRet = GetQueuedCompletionStatus(IOCP->m_hIOCompletionPort, &RecvBytes, (PULONG_PTR)&(handleInfo), (LPOVERLAPPED*)&ioInfo, INFINITE);
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
bool IOCPModel::_InitializeServerResource(bool useAcceptEX)
{
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
			this->_ShowMessage("初始化服务器监听Socket失败！\n");
			this->_Deinitialize();
			return false;
		}
		else
		{
			this->_ShowMessage("初始化服务器监听Socket完毕！\n");
		}
	}

	return true;
}

/////////////////////////////////////////////////////////////////
// 加载SOCKET库
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
	m_hIOCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	//m_WorkThreadShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (NULL == m_hIOCompletionPort)
		cout << "创建完成端口失败!\n";

	// 创建IO线程--线程里面创建线程池
	// 基于处理器的核心数量创建线程
	m_nThreads = _GetNumberOfProcessors() * 2;
	m_phWorkerThreadArray = new HANDLE[m_nThreads];
	for (DWORD i = 0; i <m_nThreads; ++i)
	{
		// 创建服务器工作器线程，并将完成端口传递到该线程
		HANDLE hWorkThread = CreateThread(NULL, NULL, _WorkerThread, (void*)this, 0, NULL);
		if (NULL == hWorkThread) {
			cerr << "创建线程句柄失败！Error:" << GetLastError() << endl;
			system("pause");
			return false;
		}
		m_phWorkerThreadArray[i] = hWorkThread;
		CloseHandle(hWorkThread);
	}
	this->_ShowMessage("完成创建工作者线程数量：%d 个\n", m_nThreads);
	return true;	
}


/////////////////////////////////////////////////////////////////
// 初始化服务器Socket
bool IOCPModel::_InitializeListenSocket()
{
	// 创建用于监听的 Listen Socket Context

	m_ListenSockInfo = new PER_SOCKET_DATA;
	m_ListenSockInfo->m_Sock.socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if(INVALID_SOCKET == m_ListenSockInfo->m_Sock.socket)
	{ 
		this->_ShowMessage("创建监听socket失败!\n");
		return false;
	}
	//listen socket与完成端口绑定
	if (NULL == CreateIoCompletionPort((HANDLE)m_ListenSockInfo->m_Sock.socket, m_hIOCompletionPort, (DWORD)m_ListenSockInfo, 0))
	{
		//RELEASE_SOCKET(m_ListenSockInfo->m_Sock.socket)
		this->_ShowMessage("监听socket与完成端口绑定失败!\n");
		return false;
	}
	else
	{
		this->_ShowMessage("监听socket与完成端口绑定完成!\n");
	}
	// 绑定地址&端口	
	m_ListenSockInfo->m_Sock.Bind(DEFAULT_PORT);

	// 开始进行监听
	if (SOCKET_ERROR == listen(m_ListenSockInfo->m_Sock.socket, SOMAXCONN)) {
		cerr << "监听失败. Error: " << GetLastError() << endl;
		return false;
	}
	else
	{
		printf("Listen() 完成.\n");
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
		printf("WSAIoctl 未能获取AcceptEx函数指针,错误代码: %d\n", WSAGetLastError());
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
		printf("WSAIoctl 未能获取GetAcceptExSockAddrs函数指针,错误代码: %d\n", WSAGetLastError());
		// 释放资源_Deinitialize
		return false;
	}

	for (int i = 0; i < MAX_POST_ACCEPT; i++)
	{
		// 投递accept
		LPPER_IO_DATA  pAcceptIoContext = new PER_IO_DATA;
		if (false == this->_PostAccept(pAcceptIoContext))
		{
			//m_pListenContext->RemoveContext(pAcceptIoContext);

			return false;
		}
	}
	printf("投递 %d 个AcceptEx请求完毕", MAX_POST_ACCEPT);
	return true;
}

/////////////////////////////////////////////////////////////////
// 创建服务器套接字
bool IOCPModel::_InitializeServerSocket()
{
	m_ServerSock.CreateSocket();
	m_ServerSock.Bind(DEFAULT_PORT);

	// 开始监听
	if (SOCKET_ERROR == listen(m_ServerSock.socket, SOMAXCONN)) {
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
	RELEASE_HANDLE(m_hWorkerShutdownEvent);

	// 释放工作者线程句柄指针
	for (int i = 0; i < m_nThreads; i++)
	{
		RELEASE_HANDLE(m_phWorkerThreadArray[i]);
	}
	delete[] m_phWorkerThreadArray;

	// 关闭IOCP句柄
	RELEASE_HANDLE(m_hIOCompletionPort);

	// 关闭服务器socket
	// ！！！！！还没写 释放 WinSocket    m_ServerSocket;

	this->_ShowMessage("释放资源完毕！\n");
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
	std::cout << msg;
}

/////////////////////////////////////////////////////////////////
// 投递I/O请求
bool IOCPModel::_PostAccept(PER_IO_DATA *pAcceptIoContext)
{
	ASSERT(ioinfo != m_ListenSockInfo.m_sock.socket);
	
	//准备参数
	// 与accept的区别,为新连接的客户端准备好socket
	DWORD dwBytes = 0;
	pAcceptIoContext->m_OpType = ACCEPT_POSTED;
	pAcceptIoContext->m_AcceptSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == pAcceptIoContext->m_AcceptSocket)
	{
		printf("创建用于Accept的Socket失败！错误代码: %d", WSAGetLastError());
		return false;
	}

	// 投递AccpetEX
	//cout << "Process AcceptEx function wait for client connect..." << endl;
	if (FALSE == m_lpfnAcceptEx(
		m_ListenSockInfo->m_Sock.socket,
		pAcceptIoContext->m_AcceptSocket,
		pAcceptIoContext->m_wsaBuf.buf,
		0,
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
		&dwBytes,
		&pAcceptIoContext->m_Overlapped))
	{
		if (ERROR_IO_PENDING != WSAGetLastError())
		{
			printf("投递 AcceptEx 请求失败，错误代码: %d", WSAGetLastError());
			return false;
		}
	}
	return true;
}

bool IOCPModel::_PostRecv(PER_SOCKET_DATA* phd, PER_IO_DATA *pid)
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

bool IOCPModel::_PostSend(PER_SOCKET_DATA* phd, PER_IO_DATA *pid)
{
	return true;
}
/////////////////////////////////////////////////////////////////
// 处理I/O请求
bool IOCPModel::_DoAccept(PER_SOCKET_DATA* handleInfo, PER_IO_DATA *ioInfo)
{
	SOCKADDR_IN *clientAddr = NULL;
	SOCKADDR_IN *localAddr = NULL;
	int clientAddrLen = sizeof(SOCKADDR_IN);
	int localAddrLen = sizeof(SOCKADDR_IN);

	// 1. 获取地址信息 （GetAcceptExSockAddrs函数不仅可以获取地址信息，还可以顺便取出第一组数据）
	m_lpfnGetAcceptExSockAddrs(ioInfo->m_wsaBuf.buf, 0, localAddrLen, clientAddrLen, (LPSOCKADDR *)&localAddr, &localAddrLen, (LPSOCKADDR *)&clientAddr, &clientAddrLen);
	//cout << "客户端：" << inet_ntoa(clientAddr->sin_addr) << ":" << ntohs(clientAddr->sin_port)<< " 连入\n";


	// 2. 为新连接建立一个SocketContext 
	PER_SOCKET_DATA *pNewSockContext = new PER_SOCKET_DATA;
	pNewSockContext->m_Sock.socket	 = ioInfo->m_AcceptSocket;
	memcpy(&(pNewSockContext->m_Sock.addr), clientAddr, sizeof(SOCKADDR_IN));

	// 3. 将新socket和完成端口绑定
	if (NULL == CreateIoCompletionPort((HANDLE)pNewSockContext->m_Sock.socket, m_hIOCompletionPort, (DWORD)pNewSockContext, 0))
	{
		DWORD dwErr = WSAGetLastError();
		if (dwErr != ERROR_INVALID_PARAMETER)
		{
			//DoClose(newSockContext);
			return false;
		}
	}

	// 4. 建立recv操作所需的ioContext，在新连接的socket上投递recv请求
	PER_IO_DATA *pNewIoContext = pNewSockContext->GetNewIOContext();
	pNewIoContext->m_OpType = RECV_POSTED;
	pNewIoContext->m_AcceptSocket = pNewSockContext->m_Sock.socket;
	// 投递recv请求
	if (false == this->_PostRecv(pNewSockContext, pNewIoContext))
	{
		//DoClose(sockContext);
		return false;
	}

	// 5. 将listenSocketContext的IOContext 重置后继续投递AcceptEx
	ioInfo->Reset();
	if (false == this->_PostAccept(ioInfo))
	{
		m_ListenSockInfo->RemoveIOContext(ioInfo);
	}



	//// 并设置tcp_keepalive
	//tcp_keepalive alive_in;
	//tcp_keepalive alive_out;
	//alive_in.onoff = TRUE;
	//alive_in.keepalivetime = 1000 * 60;		// 60s  多长时间（ ms ）没有数据就开始 send 心跳包
	//alive_in.keepaliveinterval = 1000 * 10; //10s  每隔多长时间（ ms ） send 一个心跳包
	//unsigned long ulBytesReturn = 0;
	//if (SOCKET_ERROR == WSAIoctl(pnewSockContext->m_Sock.socket, SIO_KEEPALIVE_VALS, &alive_in, sizeof(alive_in), &alive_out, sizeof(alive_out), &ulBytesReturn, NULL, NULL))
	//{
	//	//TRACE(L"WSAIoctl failed: %d/n", WSAGetLastError());
	//}


	////OnConnectionEstablished(newSockContext);



	return true;
}

bool IOCPModel::_DoRecv(PER_SOCKET_DATA* handleInfo, PER_IO_DATA *ioInfo)
{
	RecvCompleted(handleInfo, ioInfo);

	if (false == _PostRecv(handleInfo, ioInfo))
	{
		this->_ShowMessage("投递WSARecv()失败!\n");
		return false;
	}
	return true;
}

bool IOCPModel::_DoSend(PER_SOCKET_DATA* phd, PER_IO_DATA *pid)
{
	return true;
}
