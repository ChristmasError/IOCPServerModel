#pragma once

#include "csautolock_global.h"

////////////////////////////////////////////////////////
// 临界区锁
class CSAUTOLOCK_API CSLock
{ 
public:

	CSLock();
	~CSLock();

	void Lock();
	void UnLock();

private:
	CRITICAL_SECTION m_cs;
};

////////////////////////////////////////////////////////
// 自动锁

class CSAUTOLOCK_API CSAutoLock
{
public:

	CSAutoLock(CSLock& cslock);
	~CSAutoLock();

	void Lock();
	void UnLock();

private:
	CSLock & m_lock;
};