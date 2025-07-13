
#include <libcc.h>
#include <libcc/widgets/widgets.h>

#ifdef __CC_WINDOWS__
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
    case DLL_PROCESS_ATTACH:
		_cc_install_socket();
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		_cc_uninstall_socket();
		break;
	}
	return true;
}

#else

static __attribute__((constructor)) void _dynamic_attach(void) {

}

static __attribute__((destructor)) void _dynamic_detach(void) {

}
#endif