#include<IOCPModel.h>

/////////////////////////////////////////////////////////////////
// 启动服务器
bool IOCPModel::Start()
{
	SYSTEM_INFO mySysInfo;	// 确定处理器的核心数量	
	WinSocket serverSock;	// 建立服务器流式套接字
	WinSocket acceptSock; //listen接收的套接字
	LPPER_HANDLE_DATA PerSocketData;

	DWORD RecvBytes, Flags = 0;

	m_IOCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	//m_WorkThreadShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (m_IOCompletionPort == NULL)
		cout << "创建完成端口失败!\n";

	// 创建IO线程--线程里面创建线程池
	// 基于处理器的核心数量创建线程
	GetSystemInfo(&mySysInfo);
	for (DWORD i = 0; i < (mySysInfo.dwNumberOfProcessors * 2); ++i) {
		// 创建服务器工作器线程，并将完成端口传递到该线程
		HANDLE WORKThread = CreateThread(NULL, 0, _WorkerThread, (void*)this, 0, NULL);
		if (NULL == WORKThread) {
			cerr << "创建线程句柄失败！Error:" << GetLastError() << endl;
			system("pause");
			return -1;
		}
		CloseHandle(WORKThread);
	}

	serverSock.CreateSocket();
	unsigned port = 8888;
	serverSock.Bind(port);

	int listenResult = listen(serverSock.socket, 5);
	if (SOCKET_ERROR == listenResult) {
		cerr << "监听失败. Error: " << GetLastError() << endl;
		system("pause");
		return -1;
	}
	else
		cout << "本服务器已准备就绪，正在等待客户端的接入...\n";

	while (true)
	{
		//break;//测试关闭工作线程
		// 接收连接，并分配完成端，这儿可以用AcceptEx()
		acceptSock = serverSock.Accept();
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
		ZeroMemory(&(PerIoData->m_Overlapped), sizeof(WSAOVERLAPPED));
		PerIoData->m_wsaBuf.len = DATABUF_SIZE;
		PerIoData->m_wsaBuf.buf = PerIoData->m_buffer;
		PerIoData->rmMode = READ;	// read
		WSARecv(PerSocketData->m_Sock.socket, &(PerIoData->m_wsaBuf), 1, &RecvBytes, &Flags, &(PerIoData->m_Overlapped), NULL);
		//int nBytesRecv = WSARecv(PerSocketData->m_Sock.socket, &(PerIoData->m_wsaBuf), 1, &RecvBytes, &Flags, &(PerIoData->m_Overlapped), NULL);
		//if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()))
		//{
		//	cout << "如果返回值错误，并且错误的代码并非是Pending的话，那就说明这个重叠请求失败了\n";
		//	return false;
		//}
	}

	return true;
}

/////////////////////////////////////////////////////////////////
// 初始化服务器
bool IOCPModel::InitializeServer() 
{
	// 初始化退出线程事件
	m_WorkThreadShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

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
bool IOCPModel::LoadSocketLib()
{
	WSADATA wsaData;
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
	if (m_IOCompletionPort == NULL)
	{
		this->_ShowMessage("完成端口创建失败! Error：%d \n", WSAGetLastError());
		return false;
	}

	// 根据系统信息确定并建立工作者线程数量
	GetSystemInfo(&m_SysInfo);
	m_nThreads = m_SysInfo.dwNumberOfProcessors * 2;
	m_WorkerThreadsHandleArray = new HANDLE[m_nThreads];
	for (DWORD i = 0; i < m_nThreads; ++i)
	{
		HANDLE WORKTHREAD = CreateThread(NULL, 0, _WorkerThread, (LPVOID)this, 0, NULL);
		m_WorkerThreadsHandleArray[i] = WORKTHREAD;
		if (NULL == WORKTHREAD)
		{
			this->_ShowMessage("创建线程句柄失败！Error：%d \n", GetLastError());
			system("pause");
			return -1;
		}
		CloseHandle(WORKTHREAD);
	}
	this->_ShowMessage("完成创建工作者线程数量：%d 个\n", m_nThreads);

	return true;	
}


/////////////////////////////////////////////////////////////////
// 初始化服务器Socket
bool IOCPModel::_InitializeListenSocket()
{
	m_ServerSocket.CreateSocket();
	m_ServerSocket.port=DEFAULT_PORT;
	m_ServerSocket.Bind(m_ServerSocket.port);
	
	if (SOCKET_ERROR == listen(m_ServerSocket.socket, 5/*SOMAXCONN*/))
	{
		this->_ShowMessage("监听失败，Error：%d \n", GetLastError());
		return false;
	}
	
	return true;
}


/////////////////////////////////////////////////////////////////
// 释放所有资源
void IOCPModel::_Deinitialize()
{
	// 关闭系统退出事件句柄
	RELEASE_HANDLE(m_WorkThreadShutdownEvent);

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
// 工作线程函数
 DWORD WINAPI IOCPModel::_WorkerThread(LPVOID lpParam)
{
	 IOCPModel* IOCP = (IOCPModel*)lpParam;
	 LPPER_HANDLE_DATA handleInfo = NULL;
	 LPPER_IO_DATA ioInfo = NULL;
	 //用于WSARecv()
	 DWORD RecvBytes;
	 DWORD Flags = 0;

	 XHttpResponse res;     //用于处理http请求
	 while (true)
	 {

		 bool bRet = GetQueuedCompletionStatus(IOCP->m_IOCompletionPort, &RecvBytes, (PULONG_PTR)&(handleInfo),(LPOVERLAPPED*)&ioInfo, INFINITE);
		 if (bRet==false)
		 {
			 cerr << "GetQueuedCompletionStatus Error: " << GetLastError() << endl;
			 handleInfo->m_Sock.Close();    //报错
			 delete handleInfo;
			 delete ioInfo;
			 continue;
		 }
		 //收到退出该标志，直接退出工作线程
		 //if (ioInfo->m_OpType==NULL_POSTED) {
			// break;
		 //}

		 //客户端调用closesocket正常退出
		 if (RecvBytes == 0) {
			 handleInfo->m_Sock.Close();
			 delete handleInfo;
			 delete ioInfo;
			 continue;
		 }
		 //客户端直接退出，64错误,指定的网络名不可再用
		 if ((GetLastError() == WAIT_TIMEOUT) || (GetLastError() == ERROR_NETNAME_DELETED))
		 {
			 handleInfo->m_Sock.Close();
			 delete handleInfo;
			 delete ioInfo;
			 continue;
		 }
		 switch (ioInfo->rmMode)
		 {
		 case WRITE:
			 break;
			 //case WRITE
		 case READ:
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
	 }//while
	 cout << "工作线程退出！" << endl;
	 return 0;
}


/////////////////////////////////////////////////////////////////
// 打印消息
void IOCPModel::_ShowMessage(const char* msg, ...) const
{
	std::cout << msg;
}

