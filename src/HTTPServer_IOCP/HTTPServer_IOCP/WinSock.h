#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS 
#pragma warning(disable : 4996) 

#include<thread>
#include<winsock2.h>
#include<iostream>

#pragma comment(lib, "ws2_32.lib")
#define socklen_t int

class WinSock
{
public:
	WinSock();
	virtual ~WinSock();

	bool		LoadSocketLib();
	int			CreateSocket();
	int  		CreateWSASocket();
	bool		Bind(unsigned short port);
	bool		Connect(const char*ip, unsigned short port, int timeout = 1000);
	WinSock		Accept();
	int			Recv(char* buf, int bufsize);
	int			Send(const char* buf, int sendsize);
	void		Close();
	bool		SetBlock(bool isblock);
	void        GetLocalIP();

	int					socket;
	SOCKADDR_IN         addr;
	char*				ip;
	unsigned short		port;
	WSADATA				wsaData;
private:
	void _ShowMessage(const char* ch, ...) const;
};