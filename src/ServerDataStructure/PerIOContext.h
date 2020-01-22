#pragma once

#include "serverdatastructure_global.h"

// DATABUF默认大小
#define BUF_SIZE (163840)


//====================================================================================
//
//			在完成端口上投递的I/O操作的类型
//
//====================================================================================
typedef enum SERVERDATASTRUCTURE_API _OPERATION_TYPE
{
	ACCEPT_POSTED,		// 标志投递的是接收操作
	RECV_POSTED,		// 标志投递的是接收操作
	SEND_POSTED,		// 标志投递的是发送操作
	NULL_POSTED			// 初始化用
} OPERATION_TYPE;

//====================================================================================
//
//				单IO数据结构体定义(用于每一个重叠操作的参数)
//
//====================================================================================
typedef class SERVERDATASTRUCTURE_API _PER_IO_CONTEXT
{
public:
	WSAOVERLAPPED	m_Overlapped;					// OVERLAPPED结构，该结构里边有一个event事件对象,必须放在结构体首位，作为首地址
	SOCKET			m_AcceptSocket;					// 此IO操作对应的socket
	WSABUF			m_wsaBuf;						// WSABUF结构，包含成员:一个指针指向buf，和一个buf的长度len
	char			m_buffer[BUF_SIZE];			    // WSABUF具体字符缓冲区
	OPERATION_TYPE  m_OpType;						// 标志位

	_PER_IO_CONTEXT()
	{
		ZeroMemory(&(m_Overlapped), sizeof(m_Overlapped));
		ZeroMemory(m_buffer, BUF_SIZE);
		m_AcceptSocket = INVALID_SOCKET;
		m_wsaBuf.buf   = m_buffer;
		m_wsaBuf.len   = BUF_SIZE;
		m_OpType       = NULL_POSTED;
	}
	// 释放
	~_PER_IO_CONTEXT()
	{
		if (m_AcceptSocket != INVALID_SOCKET)
		{
			closesocket(m_AcceptSocket);
			m_AcceptSocket = INVALID_SOCKET;
		}
	}
	// 重置
	void Reset()
	{
		ZeroMemory(m_buffer, BUF_SIZE);
		ZeroMemory(&m_Overlapped, sizeof(m_Overlapped));
		m_OpType = NULL_POSTED;
	}

} PER_IO_CONTEXT, *LPPER_IO_CONTEXT;

