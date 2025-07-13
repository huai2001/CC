#include <libcc/types.h>

#ifdef __CC_WINDOWS__
//  从 Windows 头文件中排除极少使用的信息
#define WIN32_LEAN_AND_MEAN
// Windows 头文件:
#include <windows.h>

// dllmain.c : 定义 DLL 应用程序的入口点。
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return true;
}

#else

static __attribute__((constructor)) void _dynamic_attach(void)
{

}

static __attribute__((destructor)) void _dynamic_detach(void)
{

}
#endif