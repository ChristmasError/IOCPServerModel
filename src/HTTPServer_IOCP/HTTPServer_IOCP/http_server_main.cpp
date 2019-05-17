#include "iocp_model_class.h"
#include "http_response_class.h"
#include "clog_class.h"
using namespace std;
class HTTPServer :public IOCPModel
{
public:
	HTTPServer()
	{
		CREATE_LOG("HTTPServerLog.log", ".\\log\\");
	}
private:
	// 新连接
	void ConnectionEstablished(LPPER_SOCKET_CONTEXT socketInfo)
	{
		LOG_INFO("[Connect] Accept a connection from:%s ,Current connects:%d\n", socketInfo->m_Sock.ip, GetConnectCnt());
		return;
	}
	// 连接关闭
	void ConnectionClosed(LPPER_SOCKET_CONTEXT socketInfo)
	{
		LOG_INFO("[Disconnect] A connection closed ip:%s ,Current connects:%d\n", socketInfo->m_Sock.ip, GetConnectCnt());
		return;
	}
	// 连接发生错误
	void ConnectionError(LPPER_SOCKET_CONTEXT socketInfo, DWORD errorNum)
	{
		LOG_WARN("[Error] A connection error:%d from ip:%s, Current connects:%d\n", errorNum , socketInfo->m_Sock.ip ,GetConnectCnt());
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
		if (!res.SetRequest(ioInfo->m_buffer,responseType)) {
			LOG_WARN("[Error] Set request failed! client ip:%s\n",socketInfo->m_Sock.ip);
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
				LOG_WARN("[Error] Send http headers failed! client ip:%s\n",socketInfo->m_Sock.ip) ;
				//DoClose(sockContext);
				return;
			}
		}
		// 投递SEND，发送正文数据
		while(1)
		{
			char buf[10240];
			//将客户端请求的文件存入buf中并返回文件长度_len
			int file_len = res.Read(ioInfo->m_buffer, 102400);
			ioInfo->m_wsaBuf.len = file_len;
			if (file_len <= 0)
			{
				if (file_len == 0) // 发送完毕
				{
					LOG_INFO("[Recv] Complete response to '%s' request from ip:%s\n", responseType.c_str(), socketInfo->m_Sock.ip);
					return;
				}
				else if (file_len < 0) // 发送发生错误
				{
					LOG_WARN("[Error] Send file happen wrong! client close! client ip:%s", socketInfo->m_Sock.ip);
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
						LOG_WARN("[Error] Send data failed! client ip:%s\n", socketInfo->m_Sock.ip);
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
		LOG_INFO("[Send] Send data successd,client ip:%s\n",SocketInfo->m_Sock.ip);
		return;
	}

};
int main()
{
	HTTPServer IOCPserver;

	IOCPserver.StartServer();
	//IOCPserver.StopServer();

	return 0;
}