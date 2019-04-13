//#include"WinSocket.h"
//#include"XHttpResponse.h"
//#include<thread>
//#include<iostream>
//#include <WinSock2.h>
//#include <vector>
//#include<queue>
//#include<cstdlib>
//
//using namespace std;
//
//#pragma comment(lib, "Ws2_32.lib")		// Socket编程需用的动态链接库
//#pragma comment(lib, "Kernel32.lib")	// IOCP需要用到的动态链接库
//
//#define READ 3
//#define WRITE 5
//
//typedef struct {
//	WinSocket socket;//客户端socket
//}PER_HANDLE_DATA;
//
////重叠IO用的结构体，临时记录IO数据
//typedef struct {
//	WSAOVERLAPPED Overlapped;   //OVERLAPPED结构，该结构里边有一个event事件对象,必须放在结构体首位，作为首地址
//								//以下可自定义扩展
//	WSABUF DataBuf;				//WSABUF结构，包含成员：一个指针指向buf，和一个buf的长度len
//	char buffer[10240];			//消息数据
//	DWORD rmMode;		//标志位READ OR WRITE
//}PER_IO_DATA;
//
//typedef struct SEND_INFO {
//	PER_HANDLE_DATA handleInfo;
//	PER_IO_DATA ioInfo;
//	SEND_INFO(PER_HANDLE_DATA h, PER_IO_DATA l) :handleInfo(h), ioInfo(l) {
//	};
//};
//
//queue<SEND_INFO>Queue;
////vector < PER_IO_SOCKET* > clientGroup;		// 记录客户端的向量组
//
//HANDLE hMutex = CreateMutex(NULL, FALSE, NULL);
//
//DWORD WINAPI ServerWorkThread(LPVOID completionPort);
//DWORD WINAPI ServerSendThread(LPVOID IpParam);
//
//int main(int argc, char* argv[])
//{
//	HANDLE completionPort;
//	SYSTEM_INFO mySysInfo;	// 确定处理器的核心数量	
//	WinSocket serverSock;	// 建立服务器流式套接字
//	WinSocket acceptSock; //listen接收的套接字
//	PER_HANDLE_DATA* PerSocketData;
//
//	DWORD RecvBytes, Flags = 0;
//	// 创建IOCP的内核对象
//	/**
//	* 需要用到的函数的原型：
//	* HANDLE WINAPI CreateIoCompletionPort(
//	*    __in   HANDLE FileHandle,		// 已经打开的文件句柄或者空句柄，一般是客户端的句柄
//	*    __in   HANDLE ExistingCompletionPort,	// 已经存在的IOCP句柄
//	*    __in   ULONG_PTR CompletionKey,	// 完成键，包含了指定I/O完成包的指定文件
//	*    __in   DWORD NumberOfConcurrentThreads // 真正并发同时执行最大线程数，一般推介是CPU核心数*2
//	* );
//	**/
//	completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
//	if (completionPort == NULL)
//		cout << "创建完成端口失败!\n";
//
//	// 创建IOCP线程--线程里面创建线程池
//
//	GetSystemInfo(&mySysInfo);
//
//	// 基于处理器的核心数量创建线程
//	for (DWORD i = 0; i < (mySysInfo.dwNumberOfProcessors * 2); ++i) {
//		// 创建服务器工作器线程，并将完成端口传递到该线程
//		// DWORD WINAPI ServerWorkThread(LPVOID completionPort)
//		HANDLE WORKThread = CreateThread(NULL, 0, ServerWorkThread, completionPort, 0, NULL);
//		if (NULL == WORKThread) {
//			cerr << "创建线程句柄失败！Error:" << GetLastError() << endl;
//			system("pause");
//			return -1;
//		}
//		CloseHandle(WORKThread);
//	}
//	//                   (mySysInfo.dwNumberOfProcessors)
//	for (DWORD i = 0; i <(mySysInfo.dwNumberOfProcessors); ++i) {
//		HANDLE SENDThread = CreateThread(NULL, 0, ServerSendThread, 0, 0, NULL);
//		if (NULL == SENDThread) {
//			cerr << "创建发送句柄失败！Error:" << GetLastError() << endl;
//			system("pause");
//			return -1;
//		}
//		CloseHandle(SENDThread);
//	}
//
//	serverSock.CreateSocket();
//	/*u_long mode = 1;
//	ioctlsocket(httpserver.socket, FIONBIO,&mode );*/
//	unsigned port = 8888;
//	if (argc > 1)
//		port = atoi(argv[1]);
//	serverSock.Bind(port);
//
//	int listenResult = listen(serverSock.socket, 5);
//	if (SOCKET_ERROR == listenResult) {
//		cerr << "监听失败. Error: " << GetLastError() << endl;
//		system("pause");
//		return -1;
//	}
//	else
//		cout << "本服务器已准备就绪，正在等待客户端的接入...\n";
//
//	//创建用于发送数据的线程
//	//HANDLE sendThread = CreateThread(NULL, 0, ServerSendThread, 0, 0, NULL);
//
//	while (true)
//	{
//		// 接收连接，并分配完成端，这儿可以用AcceptEx()
//		acceptSock = serverSock.Accept();
//		acceptSock.SetBlock(false);
//		if (INVALID_SOCKET == acceptSock.socket)
//		{
//			if (WSAGetLastError() == WSAEWOULDBLOCK)
//				continue;
//			else
//			{
//				cerr << "Accept Socket Error: " << GetLastError() << endl;
//				system("pause");
//				return -1;
//			}
//		}
//
//		PerSocketData = new PER_HANDLE_DATA();
//		PerSocketData->socket = acceptSock;
//
//		CreateIoCompletionPort((HANDLE)(PerSocketData->socket.socket), completionPort, (DWORD)PerSocketData, 0);
//		//接受的套接字与完成端口关联
//
//		// 开始在接受套接字上处理I/O使用重叠I/O机制,在新建的套接字上投递一个或多个异步,WSARecv或WSASend请求
//		// 这些I/O请求完成后，工作者线程会为I/O请求提供服务	
//		// 单I/O操作数据(I/O重叠)
//		PER_IO_DATA* PerIoData = new PER_IO_DATA();
//		ZeroMemory(&(PerIoData->Overlapped), sizeof(WSAOVERLAPPED));
//		PerIoData->DataBuf.len = 1024;
//		PerIoData->DataBuf.buf = PerIoData->buffer;
//		PerIoData->rmMode = READ;	// read
//		WSARecv(PerSocketData->socket.socket, &(PerIoData->DataBuf), 1, &RecvBytes, &Flags, &(PerIoData->Overlapped), NULL);
//	}
//	serverSock.Close();
//	acceptSock.Close();
//	WSACleanup();
//	return 0;
//}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//DWORD WINAPI ServerWorkThread(LPVOID IpParam)
//{
//	HANDLE CompletionPort = (HANDLE)IpParam;
//	DWORD BytesTrans;
//	LPOVERLAPPED IpOverlapped;
//	PER_HANDLE_DATA* handleInfo = NULL;
//	PER_IO_DATA* ioInfo = NULL;
//	//用于WSARecv()
//	DWORD RecvBytes;
//	DWORD Flags = 0;
//
//	BOOL bRet = false;
//
//	while (true)
//	{
//		if (GetQueuedCompletionStatus(
//			CompletionPort,					//已完成IO信息的完成端口句柄
//			&BytesTrans,					//用于保存I/O过程中川水数据大小的变量地址
//			(PULONG_PTR)&(handleInfo),		//保存CreateIoCompletionPort()第三个参数的变量地址值
//			(LPOVERLAPPED*)&ioInfo,			//保存调用WSASend(),WSARecv()时传递的OVERLAPPED结构体地址的变量地址值
//			INFINITE)						//超时信息
//			== false)
//		{
//			cerr << "GetQueuedCompletionStatus Error: " << GetLastError() << endl;
//			return -1;
//		}
//		if (ioInfo->rmMode == WRITE)
//			continue;
//		if (0 == BytesTrans) {
//			closesocket(handleInfo->socket.socket);
//			//GlobalFree(handleInfo);//堆已损坏
//			//GlobalFree(ioInfo);
//			//delete handleInfo;
//			//delete ioInfo;
//			continue;
//		}// 检查在套接字上是否有错误发生
//
//
//		if (ioInfo->rmMode == READ)
//		{
//			Queue.push(SEND_INFO(*handleInfo, *ioInfo));
//
//		}//if READ
//
//		 // 为下一个重叠调用建立单I/O操作数据
//		ZeroMemory(&(ioInfo->Overlapped), sizeof(OVERLAPPED)); // 清空内存
//		ioInfo->DataBuf.len = 1024;
//		ioInfo->DataBuf.buf = ioInfo->buffer;
//		ioInfo->rmMode = READ;
//		WSARecv(handleInfo->socket.socket,		//client socket
//			&(ioInfo->DataBuf),				//*WSABUF
//			1,								//number of BUFFER	
//			&RecvBytes,						//接受的字节数
//			&Flags,							//标志位
//			&(ioInfo->Overlapped),			//overlaopped结构地址
//			NULL);							//没啥用
//	}//while
//
//}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//DWORD WINAPI ServerSendThread(LPVOID IpParam)
//{
//	XHttpResponse res;     //用于处理http请求
//	while (1)
//	{
//		WaitForSingleObject(hMutex, INFINITE);
//		//EnterCriticalSection(&g_cs);
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
//			//LeaveCriticalSection(&g_cs);
//			continue;
//		}
//
//	}
//	return 0;
//};