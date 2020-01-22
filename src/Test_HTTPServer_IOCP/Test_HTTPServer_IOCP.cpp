// Test_HTTPServer_IOCP.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "IOCPServerModel.h"
#include "HTTPResponse.h"
#include "MCLog.h"
using namespace std;
class HTTPServer :public IOCPModel
{
public:
	HTTPServer()
	{
		SET_LOGPATH(".\\TestServerLog\\");
	}
private:
	// 新连接
	void ConnectionEstablished(LPPER_SOCKET_CONTEXT socketInfo)
	{
		//WRITE_LOG("Connect.txt","Accept a connection from:%s ,Current connects:%d\n", socketInfo->m_Sock.ip, GetConnectCnt());
		return;
	}
	// 连接关闭
	void ConnectionClosed(LPPER_SOCKET_CONTEXT socketInfo)
	{
		//WRITE_LOG("[Disconnect] A connection closed ip:%s ,Current connects:%d\n", socketInfo->m_Sock.ip, GetConnectCnt());
		return;
	}
	// 连接发生错误
	void ConnectionError(LPPER_SOCKET_CONTEXT socketInfo, DWORD errorNum)
	{
		//WRITE_LOG("[Error] A connection error from ip:%s, Current connects:%d\n", socketInfo->m_Sock.ip, GetConnectCnt());
	}
	// Recv操作完毕
	void RecvCompleted(LPPER_SOCKET_CONTEXT socketInfo, LPPER_IO_CONTEXT ioInfo)
	{
		HttpResponse res;     //用于处理http请求
		string responseType;
		string head;

		//以下处理GET请求
		int buflend = strlen(ioInfo->m_buffer);
		if (buflend <= 0) {
			return;
		}
		if (!res.SetRequest(ioInfo->m_buffer, responseType)) {
			//WRITE_LOG("[Error] Set request failed! client ip:%s\n", socketInfo->m_Sock.ip);
			return;
		}

		// 投递SEND，发送消息头
		head = res.GetHead();
		strcpy_s(ioInfo->m_buffer, head.c_str());
		ioInfo->m_wsaBuf.len = head.size();
		if (PostSend(socketInfo, ioInfo) == false)
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				//WRITE_LOG("[Error] Send http headers failed! client ip:%s\n", socketInfo->m_Sock.ip);
				//DoClose(sockContext);
				return;
			}
		}
		// 投递SEND，发送正文数据
		while (1)
		{
			char buf[10240];
			//将客户端请求的文件存入buf中并返回文件长度_len
			int file_len = res.Read(ioInfo->m_buffer, 102400);
			ioInfo->m_wsaBuf.len = file_len;
			if (file_len <= 0)
			{
				if (file_len == 0) // 发送完毕
				{
					//WRITE_LOG("[Recv] Complete response to '%s' request from ip:%s\n", responseType.c_str(), socketInfo->m_Sock.ip);
					return;
				}
				else if (file_len < 0) // 发送发生错误
				{
					//WRITE_LOG("[Error] Send file happen wrong! client close! client ip:%s", socketInfo->m_Sock.ip);
					socketInfo->m_Sock.Close();
					delete socketInfo;
					delete ioInfo;
					return;
				}
			}
			else
			{
				if (PostSend(socketInfo, ioInfo) == false)
				{
					if (WSAGetLastError() != WSA_IO_PENDING)
					{
						//WRITE_LOG("[Error] Send data failed! client ip:%s\n", socketInfo->m_Sock.ip);
						//DoClose(sockContext);
						return;
					}
				}
			}
		}
	}
	// Send操作完毕
	void SendCompleted(LPPER_SOCKET_CONTEXT SocketInfo, LPPER_IO_CONTEXT ioInfo)
	{
		//WRITE_LOG("[Send] Send data successd,client ip:%s\n", SocketInfo->m_Sock.ip);
		return;
	}

};


int main()
{
	LOG_INIT();
	WRITE_LOG("TestLog.txt", "Start Test!!!!!!!!");
	HTTPServer IOCPserver;

	if (IOCPserver.StartServer())
	{
		cout << "=======================================\n================服务器开启=================\n=======================================\n";
	}

	//IOCPserver.StopServer();
	while (1);
	return 0;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
