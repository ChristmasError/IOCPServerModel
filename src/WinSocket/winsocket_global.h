#ifdef WINSOCKET_EXPORTS
#define WINSOCKET_API __declspec(dllexport)
#else
#define WINSOCKET_API __declspec(dllimport)
#endif

#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
#include <iostream>
#include <windows.h>
#include <WinSock2.h>
