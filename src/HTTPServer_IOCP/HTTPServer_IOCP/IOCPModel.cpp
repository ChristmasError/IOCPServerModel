#include<IOCPModel.h>

/////////////////////////////////////////////////////////////////
// 启动服务器
bool IOCPModel::Start()
{
	// 服务器运行状态检测
	if (_ServerRunning == RUNNING) {
		_ShowMessage("服务器运行中,请勿重复运行！\n");
		return false;
	}
	else
		_ServerRunning = RUNNING;

	WinSocket acceptSock; //listen接收的套接字
	LPPER_HANDLE_DATA PerSocketData;

	DWORD RecvBytes, Flags = 0;

	while (true)
	{
		// 接收连接，并分配完成端，这儿可以用AcceptEx()
		acceptSock = m_ServerSocket.Accept();
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

		PerSocketData = new PER_HANDLE_DATA();
		PerSocketData->m_Sock = acceptSock;

		CreateIoCompletionPort((HANDLE)(PerSocketData->m_Sock.socket), m_IOCompletionPort, (DWORD)PerSocketData, 0);

		// 开始在接受套接字上处理I/O使用重叠I/O机制,在新建的套接字上投递一个或多个异步,WSARecv或WSASend请求
		// 这些I/O请求完成后，工作者线程会为I/O请求提供服务	
		// 单I/O操作数据(I/O重叠)
		PER_IO_DATA* PerIoData = new PER_IO_DATA();
		PerIoData->m_OpType = RECV_POSTED;	// read
		int nBytesRecv = WSARecv(PerSocketData->m_Sock.socket, &(PerIoData->m_wsaBuf), 1, &RecvBytes, &Flags, &(PerIoData->m_Overlapped), NULL);
		if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()))
		{
			cout << "如果返回值错误，并且错误的代码并非是Pending的话，那就说明这个重叠请求失败了\n";
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

	LPPER_HANDLE_DATA handleInfo = NULL;
	LPPER_IO_DATA ioInfo = NULL;
	DWORD RecvBytes;
	DWORD Flags = 0;

	XHttpResponse res;     //用于处理http请求
	while (WaitForSingleObject(IOCP->m_WorkerShutdownEvent,0) != WAIT_OBJECT_0)
	{
		bool bRet = GetQueuedCompletionStatus(IOCP->m_IOCompletionPort, &RecvBytes, (PULONG_PTR)&(handleInfo), (LPOVERLAPPED*)&ioInfo, INFINITE);
		//收到退出线程标志，直接退出工作线程
		if (EXIT_CODE == (DWORD)handleInfo)
		{
			break;
		}
		// NULL_POSTED 操作未初始化
		if (ioInfo->m_OpType == NULL_POSTED) {
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
		// GetQueuedCompletionStatus()无错误返回
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
				case SEND_POSTED:
					break;
					//case WRITE
				case RECV_POSTED:
					bool error = false;
					bool sendfinish = false;
					for (;;)
					{
						//以下处理GET请求
						int buflend = strlen(ioInfo->m_buffer);

						if (buflend <= 0) {
							break;
						}
						if (!res.SetRequest(ioInfo->m_buffer)) {
							cerr << "SetRequest failed!" << endl;
							error = true;
							break;
						}
						string head = res.GetHead();
						if (handleInfo->m_Sock.Send(head.c_str(), head.size()) <= 0)
						{
							break;
						}//回应报头
						for (;;)
						{
							char buf[10240];
							//将客户端请求的文件存入buf中并返回文件长度_len
							int file_len = res.Read(buf, 10240);
							if (file_len == 0)
							{
								sendfinish = true;
								break;
							}
							if (file_len < 0)
							{
								error = true;
								break;
							}
							if (handleInfo->m_Sock.Send(buf, file_len) <= 0)
							{
								break;
							}
						}//for(;;)
						if (sendfinish)
						{
							break;
						}
						if (error)
						{
							cerr << "send file happen wrong ! client close !" << endl;
							handleInfo->m_Sock.Close();
							delete handleInfo;
							delete ioInfo;
							break;
						}
					}
					break;
					//case READ
				}//switch
				ioInfo->Reset();
				ioInfo->m_OpType = RECV_POSTED;	// read
				WSARecv(handleInfo->m_Sock.socket, &(ioInfo->m_wsaBuf), 1, &RecvBytes, &Flags, &(ioInfo->m_Overlapped), NULL);
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
bool IOCPModel::InitializeServerResource()
{
	// 服务器非运行
	_ServerRunning = STOP;

	// 初始化退出线程事件
	m_WorkerShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// 初始化IOCP
	if (_InitializeIOCP() == false)
	{
		this->_ShowMessage("初始化IOCP失败！\n");
		return false;
	}
	else
	{
		this->_ShowMessage("初始化IOCP完毕！\n");
	}
	// 初始化服务器socket
	if (_InitializeListenSocket() == false)
	{
		this->_ShowMessage("初始化服务器Socket失败！\n");
		this->_Deinitialize();
		return false;
	}
	else
	{
		this->_ShowMessage("初始化服务器Socket完毕！\n");
	}

	this->_ShowMessage("本服务器已准备就绪，正在等待客户端的接入......\n");
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
	if (m_IOCompletionPort == NULL)
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
	m_ServerSocket.CreateSocket();
	m_ServerSocket.port = DEFAULT_PORT;
	m_ServerSocket.Bind(m_ServerSocket.port);

	//if (NULL == CreateIoCompletionPort((HANDLE)&m_ServerSocket.socket, m_IOCompletionPort, (DWORD)&m_ServerSocket, 0))
	//{
	//	//RELEASE_SOCKET(listenSockContext->connSocket);
	//	return false;
	//}

	// 开始监听
	if (SOCKET_ERROR == listen(m_ServerSocket.socket, 5)) {
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

