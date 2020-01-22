#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS 
#pragma warning(disable : 4996) 

#include "winsocket_global.h"

#pragma comment(lib, "ws2_32.lib")
#define socklen_t int

class WINSOCKET_API WinSocket
{
public:
	SOCKET				socket;
	SOCKADDR_IN         addr;
	const char* ip;
	unsigned short		port;

	WinSocket();
	virtual ~WinSocket();

	bool		LoadSocketLib();
	void		UnloadSocketLib();
	int			CreateSocket();
	int  		CreateWSASocket();
	bool		Bind(unsigned short port);
	bool		Connect(const char*ip, unsigned short port, int timeout = 1000);
	WinSocket	Accept();
	int			Recv(char* buf, int bufsize);
	int			Send(const char* buf, int sendsize);
	void		Close();
	bool		SetBlock(bool isblock);
	const char*       GetLocalIP();

private:
	void _ShowMessage(const char* ch, ...) const;
};

