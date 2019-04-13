#include<WinSocket.h>		//封装好的socket类库,项目中需加载其dll位置在首目录的lib文件夹中
#include<WinSock2.h>
#include<thread>
#include<iostream>
#include<stdio.h>
#include"XHttpResponse.h"
using namespace std;

#pragma comment(lib, "Kernel32.lib")	// IOCP需要用到的动态链接库

#define READ 3
#define WRITE 5
#define DATABUF_SIZE 1024
#define WORK_THREADS_EXIT_CODE NULL
HANDLE WorkThreadShutdownEvent = NULL;

typedef struct {
	WinSocket socket;//客户端socket信息		 
}PER_HANDLE_DATA,*LPPER_HANDLE_DATA;

//重叠IO用的结构体，临时记录IO数据
typedef struct {
	WSAOVERLAPPED Overlapped;   //OVERLAPPED结构，该结构里边有一个event事件对象,必须放在结构体首位，作为首地址
	WSABUF DataBuf;				//WSABUF结构，包含成员：一个指针指向buf，和一个buf的长度len
	char buffer[DATABUF_SIZE];			//消息数据
	DWORD rmMode;		//标志位READ OR WRITE
}PER_IO_DATA,*LPPER_IO_DATA;

typedef struct{
	HANDLE completionPort;
	HANDLE workThreadsQuitEvent;
}IOCP_DATA;

DWORD WINAPI ServerWorkThread(LPVOID completionPort);

int main(int argc, char* argv[])
{
	//HANDLE completionPort;
	IOCP_DATA IOCP;
	SYSTEM_INFO mySysInfo;	// 确定处理器的核心数量	
	WinSocket serverSock;	// 建立服务器流式套接字
	WinSocket acceptSock; //listen接收的套接字
	LPPER_HANDLE_DATA PerSocketData;

	DWORD RecvBytes, Flags = 0;

	IOCP.completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	IOCP.workThreadsQuitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (IOCP.completionPort == NULL)
		cout << "创建完成端口失败!\n";

	// 创建IO线程--线程里面创建线程池
	// 基于处理器的核心数量创建线程
	GetSystemInfo(&mySysInfo);
	for (DWORD i = 0; i < (mySysInfo.dwNumberOfProcessors * 2); ++i) {
		// 创建服务器工作器线程，并将完成端口传递到该线程
		HANDLE WORKThread = CreateThread(NULL, 0, ServerWorkThread, (LPVOID)&IOCP, 0, NULL);
		if (NULL == WORKThread) {
			cerr << "创建线程句柄失败！Error:" << GetLastError() << endl;
			system("pause");
			return -1;
		}
		CloseHandle(WORKThread);
	}
	//for (DWORD i = 0; i <(mySysInfo.dwNumberOfProcessors ); ++i) {
	//	HANDLE SENDThread = CreateThread(NULL, 0, ServerSendThread, 0, 0, NULL);
	//	if (NULL == SENDThread) {
	//		cerr << "创建发送句柄失败！Error:" << GetLastError() << endl;
	//		system("pause");
	//		return -1;
	//	}
	//	CloseHandle(SENDThread);
	//}

	serverSock.CreateSocket();
	unsigned port = 8888;
	if (argc > 1)
		port = atoi(argv[1]);
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
		PerSocketData->socket = acceptSock;

		CreateIoCompletionPort((HANDLE)(PerSocketData->socket.socket), IOCP.completionPort, (DWORD)PerSocketData, 0);

		// 开始在接受套接字上处理I/O使用重叠I/O机制,在新建的套接字上投递一个或多个异步,WSARecv或WSASend请求
		// 这些I/O请求完成后，工作者线程会为I/O请求提供服务	
		// 单I/O操作数据(I/O重叠)
		PER_IO_DATA* PerIoData = new PER_IO_DATA();
		ZeroMemory(&(PerIoData->Overlapped), sizeof(WSAOVERLAPPED));
		PerIoData->DataBuf.len = 1024;
		PerIoData->DataBuf.buf = PerIoData->buffer;
		PerIoData->rmMode = READ;	// read
		WSARecv(PerSocketData->socket.socket, &(PerIoData->DataBuf), 1, &RecvBytes, &Flags, &(PerIoData->Overlapped), NULL);
	}
	if (serverSock.socket != INVALID_SOCKET)
	{
		WorkThreadShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		SetEvent(WorkThreadShutdownEvent);
		for(int i=0;i<(mySysInfo.dwNumberOfProcessors * 2); ++i)
		{
			PostQueuedCompletionStatus(IOCP.completionPort, 0, (DWORD)WORK_THREADS_EXIT_CODE, NULL);
		}
		ResetEvent(WorkThreadShutdownEvent);
	}
	serverSock.Close();
	acceptSock.Close();
	WSACleanup();
	while (1);
	return 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DWORD WINAPI ServerWorkThread(LPVOID IpParam)
{
	//HANDLE CompletionPort = (HANDLE)IpParam;
	IOCP_DATA* IOCP=(IOCP_DATA*)IpParam;
	DWORD BytesTrans;
	LPOVERLAPPED IpOverlapped;
	LPPER_HANDLE_DATA handleInfo=NULL;
	LPPER_IO_DATA ioInfo = NULL;
	//用于WSARecv()
	DWORD RecvBytes;
	DWORD Flags = 0;
	
	XHttpResponse res;     //用于处理http请求
	while (WAIT_OBJECT_0!=WaitForSingleObject(IOCP->workThreadsQuitEvent , 0))
	{
		//if (GetQueuedCompletionStatus(
		//			CompletionPort,					//已完成IO信息的完成端口句柄
		//			&BytesTrans,					//用于保存I/O过程中川水数据大小的变量地址
		//			(PULONG_PTR)&(handleInfo),		//保存CreateIoCompletionPort()第三个参数的变量地址值
		//			(LPOVERLAPPED*)&ioInfo,			//保存调用WSASend(),WSARecv()时传递的OVERLAPPED结构体地址的变量地址值
		//			INFINITE)						//超时信息
		//	== false)
		if (GetQueuedCompletionStatus(IOCP->completionPort,&BytesTrans,(PULONG_PTR)&(handleInfo),(LPOVERLAPPED*)&ioInfo,INFINITE)== false)
		{
			cerr << "GetQueuedCompletionStatus Error: " << GetLastError() << endl;
			handleInfo->socket.Close();
			delete handleInfo;
			delete ioInfo;
			continue;
		}
		//收到退出该标志，直接退出工作线程
		if (handleInfo == WORK_THREADS_EXIT_CODE) {
			break;
		}
			
		//客户端调用closesocket正常退出
		if (BytesTrans == 0) {
			handleInfo->socket.Close();
			delete handleInfo;
			delete ioInfo;
			continue;
		}
		//客户端直接退出，64错误,指定的网络名不可再用
		if ((GetLastError() == WAIT_TIMEOUT) || (GetLastError() == ERROR_NETNAME_DELETED))
		{
			handleInfo->socket.Close();
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
				int buflend = strlen(ioInfo->buffer);

				if (buflend <= 0) {
					break;
				}
				if (!res.SetRequest(ioInfo->buffer)) {
					cerr<<"SetRequest failed!"<<endl;
					error = true;
					break;
				}
				string head = res.GetHead();
				if (handleInfo->socket.Send(head.c_str(), head.size()) <= 0)
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
					if (handleInfo->socket.Send(buf, file_len) <= 0)
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
					cerr << "send file happen wrong ! client close !"<<endl;
					handleInfo->socket.Close();
					delete handleInfo;
					delete ioInfo;
					break;
				}
			}
			break;
			//case READ
		}//switch

		 // 为下一个重叠调用建立单I/O操作数据
		ZeroMemory(&(ioInfo->Overlapped), sizeof(OVERLAPPED)); // 清空内存
		ioInfo->DataBuf.len = 1024;
		ioInfo->DataBuf.buf = ioInfo->buffer;
		ioInfo->rmMode = READ;
		WSARecv(handleInfo->socket.socket,		//client socket
				&(ioInfo->DataBuf),				//*WSABUF
				1,								//number of BUFFER	
				&RecvBytes,						//接受的字节数
				&Flags,							//标志位
				&(ioInfo->Overlapped),			//overlaopped结构地址
				NULL);							//没啥用
	}//while
	cout << "工作线程退出！" << endl;
	return 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//DWORD WINAPI ServerSendThread(LPVOID IpParam)
//{
//	XHttpResponse res;     //用于处理http请求
//	while (1)
//	{
//		WaitForSingleObject(hMutex, INFINITE);
//		if (!Queue.empty())
//		{
//			SEND_INFO SENDINFO = Queue.front();
//			Queue.pop();
//			ReleaseMutex(hMutex);
//			//以下处理GET请求
//			char* buf = SENDINFO.ioInfo.buffer;
//			int buflend = strlen(SENDINFO.ioInfo.buffer);
//
//			if (buflend <= 0) {
//				continue;
//			}
//			if (!res.SetRequest(buf)) {
//				cout << "SetRequest failed!\n";
//				continue;
//			}
//			string head = res.GetHead();
//			if (SENDINFO.handleInfo.socket.Send(head.c_str(), head.size()) <= 0)
//			{
//				//错误处理
//				break;
//			}//回应报头
//			bool error = false;
//			for (;;)
//			{
//				//将客户端请求的文件存入buf中并返回文件长度_len
//				int file_len = res.Read(buf, buflend);
//				if (file_len < 0)
//				{
//					error = true;
//					break;
//				}
//				else if (file_len == 0) {
//					break;
//				}
//
//				memset(&(SENDINFO.ioInfo.Overlapped), 0, sizeof(OVERLAPPED));
//				SENDINFO.ioInfo.DataBuf.len = file_len;
//				SENDINFO.ioInfo.DataBuf.buf = buf;
//				SENDINFO.ioInfo.rmMode = WRITE;//WRITE
//
//				if (SENDINFO.handleInfo.socket.Send(buf, file_len) <= 0)
//				{					
//					break;
//				}
//				//WSASend(SENDINFO.handleInfo.socket.socket, &(SENDINFO.ioInfo.DataBuf), 1, NULL, 0, &(SENDINFO.ioInfo.Overlapped), NULL);
//			}
//			if (error)
//			{
//				cout << "something wrong ! client close !\n";
//				SENDINFO.handleInfo.socket.Close();
//			}
//		}
//		else
//		{
//			ReleaseMutex(hMutex);
//			continue;
//		}
//
//	}//while
//	return 0;
//};
