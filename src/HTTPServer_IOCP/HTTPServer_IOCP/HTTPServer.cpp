#include<IOCPModel.h>
using namespace std;
class HTTPServer :public IOCPModel
{
	// 重载RecvCompleted()
	void RecvCompleted(LPPER_SOCKET_CONTEXT SocketInfo, LPPER_IO_CONTEXT IoInfo)
	{
		HttpResponse res;     //用于处理http请求

		bool error = false;
		bool sendfinish = false;
		//以下处理GET请求
		int buflend = strlen(IoInfo->m_buffer);
		if (buflend <= 0) {
			return;
		}
		if (!res.SetRequest(IoInfo->m_buffer)) {
			cerr << "SetRequest failed!" << endl;
			error = true;
			return;
		}
		
		string head = res.GetHead();
		
		// 投递SEND，发送头
		strcpy_s(IoInfo->m_buffer, head.c_str());
		IoInfo->m_wsaBuf.len = head.size();
		if (PostSend(SocketInfo,IoInfo)==false)
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				cout << "send false!\n";
				//DoClose(sockContext);
				return ;
			}
		}
		int i = 0;
		for (;;i++)
		{
			char buf[10240];
			//将客户端请求的文件存入buf中并返回文件长度_len
			int file_len = res.Read(IoInfo->m_buffer, 102400);
			IoInfo->m_wsaBuf.len = file_len;
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
			// 投递SEND，发送正文
			if (PostSend(SocketInfo, IoInfo) == false)
			{
				if (WSAGetLastError() != WSA_IO_PENDING)
				{
					cout << "send false!\n";
					//DoClose(sockContext);
					return;
				}
			}
		}
		if (sendfinish)
		{
			//cout << "!!!!!!!!!!!!!!!!!循环"<<i<<"次,send finish!!!!!!!!!!!!!!!!!\n";
			return;
		}
		if (error)
		{
			cerr << "send file happen wrong ! client close !" << endl;
			SocketInfo->m_Sock.Close();
			delete SocketInfo;
			delete IoInfo;
			return;
		}
	}
	// 重载SendCompleted()
	void SendCompleted(LPPER_SOCKET_CONTEXT SocketInfo, LPPER_IO_CONTEXT IoInfo)
	{
		std::cout << "发送数据成功！\n";
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