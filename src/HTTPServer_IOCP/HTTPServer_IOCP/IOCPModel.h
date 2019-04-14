#pragma once
#include<WinSocket.h>		//封装好的socket类库,项目中需加载其dll位置在首目录的lib文件夹中
#include<WinSock2.h>
#include<thread>
#include<iostream>
#include<vector>
#include<stdio.h>
#include"XHttpResponse.h"

#pragma comment(lib, "Kernel32.lib")	// IOCP需要用到的动态链接库

// 标志投递的是接收操作
#define RECV_POSTED 3
// 标志投递的是发送操作
#define SEND_POSTED 5
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
// 单IO数据结构体定义(用于每一个重叠操作的参数)
//========================================================
typedef struct
{
	WSAOVERLAPPED Overlapped;			//OVERLAPPED结构，该结构里边有一个event事件对象,必须放在结构体首位，作为首地址
	WSABUF DataBuf;						//WSABUF结构，包含成员：一个指针指向buf，和一个buf的长度len
	char buffer[DATABUF_SIZE];			//消息数据
	DWORD rmMode;						//标志位READ or WRITE
} PER_IO_DATA, *LPPER_IO_DATA;


//========================================================
// 单IO数据结构体定义(用于每一个客户端的重叠操作的参数)
//========================================================
typedef struct 
{
	WinSocket socket;					//客户端socket信息		 
} PER_HANDLE_DATA, *LPPER_HANDLE_DATA;


//========================================================
//					IOCPModel类定义
//========================================================
class IOCPModel
{
public:
	IOCPModel(void);
	~IOCPModel(void);
public:
	// 初始化服务器
	bool InitializeServer();

	// 启动服务器
	bool Start();

	// 关闭IOCPserver
	bool Stop();

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
	bool _PostRecv();

	// 线程函数，为IOCP请求服务工作者线程
	static DWORD WINAPI _WorkerThread(LPVOID lpParam);

	// 打印消息
	void _ShowMessage(const char*, ...) const;
	

private:
	SYSTEM_INFO					  m_SysInfo;					// 操作系统信息

	HANDLE					      m_WorkThreadShutdownEvent;	// 通知线程系统推出事件

	HANDLE						  m_IOCompletionPort;			// 完成端口句柄

	HANDLE*						  m_WorkerThreadsHandleArray;	// 工作者线程句柄数组

	unsigned int				  m_nThreads;				    // 工作线程数量

	WinSocket					  m_ServerSocket;				// 封装的socket类库

	vector<LPPER_HANDLE_DATA>     m_ClientContext;				// 客户端socket的context信息

	LPPER_HANDLE_DATA		      m_pListenContext;				// 用于监听客户端Socket的context信息

};

