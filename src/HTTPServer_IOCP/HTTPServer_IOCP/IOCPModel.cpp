#include<IOCPModel.h>

bool IOCPModel::InitializeServer() 
{
	// 初始化退出线程事件
	m_WorkThreadShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// 初始化IOCP
	if (_InitializeIOCP == false)
	{
		this->_ShowMessage("初始化IOCP失败！\n");
		return false;
	}
	else
	{
		this->_ShowMessage("初始化IOCP完毕！\n");
	}
	// 初始化服务器socket
	if (_InitializeListenSocket == false)
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
		HANDLE WORKTHREAD = CreateThread(NULL, 0, _WorkerThread, (LPVOID)&m_IOCompletionPort, 0, NULL);
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
	m_ServerSocket.port=atoi(DEFAULT_IP);
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
// 打印消息
void IOCPModel::_ShowMessage(const char* msg, ...) const
{
	std::cout << msg;
}