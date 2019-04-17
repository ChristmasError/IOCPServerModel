#pragma once
#include<WinSocket.h>		//封装好的socket类库,项目中需加载其dll位置在首目录的lib文件夹中
#include<WinSock2.h>
#include<thread>
#include<iostream>
#include<vector>
#include<stdio.h>
#include"XHttpResponse.h"

#pragma comment(lib, "Kernel32.lib")	// IOCP需要用到的动态链接库

// 服务器运行状态
#define RUNNING true
#define STOP false

// DATABUF默认大小
#define BUF_SIZE 1024
// 传递给Worker线程的退出信号
#define WORK_THREADS_EXIT NULL
// 默认端口
#define DEFAULT_PORT  8888
// 默认IP
#define DEFAULT_IP "10.11.147.70"
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
	WSABUF			m_wsaBuf;						// WSABUF结构，包含成员：一个指针指向buf，和一个buf的长度len
	char			m_buffer[BUF_SIZE];			// WSABUF具体字符缓冲区
	OPERATION_TYPE           m_OpType;
	//OPERATION_TYPE  m_OpType;						// 标志位

	// 初始化
	_PER_IO_DATA()
	{
		ZeroMemory(&(m_Overlapped), sizeof(WSAOVERLAPPED));
		ZeroMemory(m_buffer, BUF_SIZE);
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
// 单IO数据结构题定义(每一个客户端socket参数）
//========================================================
typedef struct  _PER_HANDLE_DATA
{
	WinSocket				m_Sock;		//每一个socket
	//vector<PER_IO_DATA*>    m_vecIoContex;  //每个socket接收到的所有IO请求数组

	// 初始化
	_PER_HANDLE_DATA()
	{
	}
	// 释放资源
	~_PER_HANDLE_DATA()
	{
	}
} PER_HANDLE_DATA,*LPPER_HANDLE_DATA;

//========================================================
//					IOCPModel类定义
//========================================================
class IOCPModel
{
public:
	// 服务器内资源初始化
	IOCPModel()
	{
		m_WorkerShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		InitializeServerResource();
	}
	~IOCPModel()
	{
		_Deinitialize();
	}
public:
	// 开启服务器
	bool Start();

	// 关闭服务器
	//void Stop();

	// 初始化服务器资源
	// 1.初始化Winsock服务
	// 2.初始化IOCP + 工作函数线程池
	// 3.监听服务器套接字
	// 4.accept()
	bool InitializeServerResource();

	// 加载Winsock库:public
	bool LoadSocketLib();

	// 卸载Socket库
	bool UnloadSocketLib() 
	{ 
		WSACleanup(); 
	}

private:
	// 加载Winsock库:private
	bool _LoadSocketLib();

	// 初始化IOCP:完成端口与工作线程线程池创建
	bool _InitializeIOCP();

	// 监听服务器套接字
	bool _InitializeListenSocket();

	// 线程函数，为IOCP请求服务工作者线程
	static DWORD WINAPI _WorkerThread(LPVOID lpParam);

	// 投递WSARecv()请求
	bool _PostRecv();

	// 投递WSARecv()请求
	//bool _PostRecv();

	// 打印消息
	void _ShowMessage(const char*, ...) const;

	// 释放所有资源
	void _Deinitialize();

protected:
	bool						  _ServerRunning;				// 服务器运行判断

	WSADATA						  wsaData;						// Winsock服务初始化

	SYSTEM_INFO					  m_SysInfo;					// 操作系统信息

	HANDLE					      m_WorkerShutdownEvent;	// 通知线程系统推出事件

	HANDLE						  m_IOCompletionPort;			// 完成端口句柄

	WinSocket					  m_ServerSocket;				// 封装的socket类库

	HANDLE*						  m_WorkerThreadsHandleArray;	// 工作者线程句柄数组

	int							  m_nThreads;				    // 工作线程数量

	

};
