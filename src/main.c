#include <libcc.h>
#ifndef __CC_WINDOWS__
#include <signal.h>
#endif

static bool_t _libcc_initialized = false;
static bool_t _libcc_cleaned = false;

#ifdef _CC_USE_OPENSSL_
#include <openssl/evp.h>
#endif

#ifndef __GNUC__
#define __attribute__(x)
#endif

__attribute__((constructor)) void _libcc_attach(void) {
    /* Prevent multiple initialization */
    if (_libcc_initialized) {
        return;
    }
    _libcc_initialized = true;

	srand((uint32_t)time(NULL));
#ifdef _CC_USE_DEBUG_MALLOC_
    _attach_debug_taracked();
#endif
    _cc_get_cpu_cores();
    _cc_install_socket();
#ifdef _CC_USE_OPENSSL_
	OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CONFIG, NULL);
#endif

	_cc_logger_debug("libcc attach");
}

__attribute__((destructor)) void _libcc_detach(void) {
    /* Prevent double cleanup */
    if (_libcc_cleaned) {
        return;
    }
    _libcc_cleaned = true;

	_cc_uninstall_socket();

#ifdef _CC_USE_DEBUG_MALLOC_
    _detach_debug_taracked();
#endif

	_cc_logger_debug("libcc detach");
}

#if defined(__CC_WINDOWS__) && defined (_CC_API_USE_DYNAMIC_)
BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved ) {
	switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
		_libcc_attach();
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		_libcc_detach();
		break;
	}
	return true;
}
#endif
