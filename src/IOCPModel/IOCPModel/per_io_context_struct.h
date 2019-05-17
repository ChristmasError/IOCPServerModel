#pragma once

#include "operation_type_enum.h"
#include <WinSock2.h>
#include <list>

// DATABUF默认大小
#define BUF_SIZE 102400

//====================================================================================
//
//				单IO数据结构体定义(用于每一个重叠操作的参数)
//
//====================================================================================
typedef struct _PER_IO_CONTEXT
{
public:
	WSAOVERLAPPED	m_Overlapped;					// OVERLAPPED结构，该结构里边有一个event事件对象,必须放在结构体首位，作为首地址
	SOCKET			m_AcceptSocket;					// 此IO操作对应的socket
	WSABUF			m_wsaBuf;						// WSABUF结构，包含成员：一个指针指向buf，和一个buf的长度len
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
		printf("释放\n");
	}
	// 重置
	void Reset()
	{
		ZeroMemory(m_buffer, BUF_SIZE);
		//ZeroMemory(&m_Overlapped, sizeof(m_Overlapped));
		//
		//if (m_wsaBuf.buf != NULL)
		//	ZeroMemory(m_wsaBuf.buf, BUF_SIZE);
		//else
		//	m_wsaBuf.buf = (char *)::HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BUF_SIZE);

		//m_wsaBuf.buf = m_buffer;
		//m_OpType = NULL_POSTED;
	}

} PER_IO_CONTEXT, *LPPER_IO_CONTEXT;

