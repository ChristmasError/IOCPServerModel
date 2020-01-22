#pragma once

#ifdef SERVERDATASTRUCTURE_EXPORTS
#define SERVERDATASTRUCTURE_API __declspec(dllexport)
#else
#define SERVERDATASTRUCTURE_API __declspec(dllimport)
#endif
#ifdef _DEBUG
#pragma comment(lib,"CSAUTOLOCKd.lib")
#pragma comment(lib,"WINSOCKETd.lib")
#else
#pragma comment(lib,"CSAUTOLOCK.lib")
#pragma comment(lib,"WINSOCKET.lib")
#endif // DEBUG


#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容

#include <iostream>
#include <WinSock2.h>
#include <vector>
#include <list>
