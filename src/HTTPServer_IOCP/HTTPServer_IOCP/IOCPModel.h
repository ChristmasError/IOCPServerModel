#pragma once
#include<WinSocket.h>		//封装好的socket类库,项目中需加载其dll位置在首目录的lib文件夹中
#include<WinSock2.h>
#include<thread>
#include<iostream>
#include<vector>
#include<stdio.h>
#include"XHttpResponse.h"

#pragma comment(lib, "Kernel32.lib")	// IOCP需要用到的动态链接库

// DATABUF默认大小
#define DATABUF_SIZE 1024
// 传递给Worker线程的退出信号
#define WORK_THREADS_EXIT_CODE NULL
// 默认端口
#define DEFAULT_PORT  8888
// 默认IP
#define DEFAULT_IP "10.11.147.70"

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

#define READ 3
#define WRITE 5
//========================================================
// 单IO数据结构体定义(用于每一个重叠操作的参数)
//========================================================
typedef struct _PER_IO_DATA
{
	WSAOVERLAPPED	m_Overlapped;					// OVERLAPPED结构，该结构里边有一个event事件对象,必须放在结构体首位，作为首地址
	//SOCKET			socket;							// 这个IO操作使用的socket
	WSABUF			m_wsaBuf;						// WSABUF结构，包含成员：一个指针指向buf，和一个buf的长度len
	char			m_buffer[DATABUF_SIZE];			// WSABUF具体字符缓冲区
	DWORD rmMode;
	//OPERATION_TYPE  m_OpType;						// 标志位

	//// 初始化
	//_PER_IO_DATA()
	//{
	//	ZeroMemory(&(m_Overlapped), sizeof(WSAOVERLAPPED));
	//	//socket = INVALID_SOCKET;
	//	ZeroMemory(m_buffer, DATABUF_SIZE);
	//	m_wsaBuf.buf = m_buffer;
	//	m_wsaBuf.len = DATABUF_SIZE;
	//	m_OpType = NULL_POSTED;
	//}
	//// 释放
	//~_PER_IO_DATA()
	//{
	//	if (socket != INVALID_SOCKET)
	//	{
	//		closesocket(socket);
	//		socket = INVALID_SOCKET;
	//	}
	//}

	// 重置buf缓冲区
	void ResetBuffer()
	{
		ZeroMemory(m_buffer, DATABUF_SIZE);
	}
} PER_IO_DATA, *LPPER_IO_DATA;

//========================================================
// 单IO数据结构题定义(每一个客户端socket参数）
//========================================================
typedef struct  _PER_HANDLE_DATA
{
	WinSocket				m_Sock;		//每一个socket
	//vector<PER_IO_DATA*>    m_vecIoContex;  //每个socket接收到的所有IO请求数组

	//// 初始化
	//_PER_HANDLE_DATA()
	//{
	//}
	//// 释放资源
	//~_PER_HANDLE_DATA()
	//{
	//	if (m_Sock.socket != INVALID_SOCKET)
	//	{
	//		m_Sock.Close();
	//		m_Sock.socket = INVALID_SOCKET;
	//	}
	//	for (int i = 0; i < m_vecIoContex.size(); i++)
	//	{
	//		delete m_vecIoContex[i];
	//	}
	//}
} PER_HANDLE_DATA,*LPPER_HANDLE_DATA;

//========================================================
//					IOCPModel类定义
//========================================================
class IOCPModel
{
public:
	//IOCPModel()
	//{
	//	InitializeServer();
	//}
	//~IOCPModel()
	//{
	//	//this->Stop();
	//}
public:
	// 启动服务器
	bool Start();

	// 关闭服务器
	//bool Stop();

	// 初始化服务器
	bool InitializeServer();

	// 加载Socket库
	bool LoadSocketLib();

	// 卸载Socket库
	bool UnloadSocketLib() 
	{ 
		WSACleanup(); 
	}

private:
	// 初始化IOCP:完成端口与工作线程线程池创建
	bool _InitializeIOCP();

	// 初始化服务器Socket
	bool _InitializeListenSocket();

	// 释放所有资源
	void _Deinitialize();

	// 投递WSARecv()请求
	//bool _PostRecv();

	// 线程函数，为IOCP请求服务工作者线程
	static DWORD WINAPI _WorkerThread(LPVOID lpParam);

	// 打印消息
	void _ShowMessage(const char*, ...) const;
	

private:
	SYSTEM_INFO					  m_SysInfo;					// 操作系统信息

	HANDLE					      m_WorkThreadShutdownEvent;	// 通知线程系统推出事件

	HANDLE						  m_IOCompletionPort;			// 完成端口句柄

	HANDLE*						  m_WorkerThreadsHandleArray;	// 工作者线程句柄数组

	int							  m_nThreads;				    // 工作线程数量

	WinSocket					  m_ServerSocket;				// 封装的socket类库

	vector<LPPER_HANDLE_DATA>     m_ClientContext;				// 客户端socket的context信息

	LPPER_HANDLE_DATA		      m_pListenContext;				// 用于监听客户端Socket的context信息

};
