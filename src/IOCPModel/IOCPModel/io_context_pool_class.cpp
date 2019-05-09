#pragma once

#include "io_context_pool_class.h"
#include <iostream>

// IOContextPool中的初始数量
#define INIT_IOCONTEXT_NUM (100)

IOContextPool::IOContextPool()
{

	contextList.clear();

	CSAutoLock csl(m_csLock);
	for (size_t i = 0; i < INIT_IOCONTEXT_NUM; i++)
	{
		LPPER_IO_CONTEXT context = new PER_IO_CONTEXT;
		contextList.push_back(context);
	}

	std::cout << "IOContextPool 初始化完成\n";
}

IOContextPool::~IOContextPool()
{
	CSAutoLock csl(m_csLock);
	for (std::list<LPPER_IO_CONTEXT>::iterator it = contextList.begin(); it != contextList.end(); it++)
	{
		delete (*it);
	}
	contextList.clear();
}

LPPER_IO_CONTEXT IOContextPool::AllocateIoContext()
{
	LPPER_IO_CONTEXT context = NULL;

	CSAutoLock csl(m_csLock);
	if (contextList.size() > 0) //list不为空，从list中取一个
	{
		context = contextList.back();
		contextList.pop_back();
	}
	else	//list为空，新建一个
	{
		context = new PER_IO_CONTEXT;
	}

	return context;
}

void IOContextPool::ReleaseIOContext(LPPER_IO_CONTEXT pContext)
{
	pContext->Reset();

	CSAutoLock csl(m_csLock);
	contextList.push_front(pContext);

}