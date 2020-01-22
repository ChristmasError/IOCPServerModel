#pragma once

#ifdef CSAUTOLOCK_EXPORTS
#define CSAUTOLOCK_API __declspec(dllexport)
#else
#define CSAUTOLOCK_API __declspec(dllimport)
#endif


#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
#define _WINSOCKAPI_
#include <windows.h>
