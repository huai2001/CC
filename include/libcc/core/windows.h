/*
 * Copyright libcc.cn@gmail.com. and other libcc contributors.
 * All rights reserved.org>
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:

 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
*/
#ifndef _C_CC_WINDOWS_H_INCLUDED_
#define _C_CC_WINDOWS_H_INCLUDED_

#if !defined(_WIN64) && !defined(_WIN32)
#error Unsupported OS
#endif

#include "../types.h"

// force_align_arg_pointer attribute requires gcc >= 4.2.x.
#if defined(__clang__)
#define HAVE_FORCE_ALIGN_ARG_POINTER
#elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 2))
#define HAVE_FORCE_ALIGN_ARG_POINTER
#endif

#if defined(__GNUC__) && defined(__i386__) && defined(HAVE_FORCE_ALIGN_ARG_POINTER)
#define MINGW32_FORCEALIGN __attribute__((force_align_arg_pointer))
#else
#define MINGW32_FORCEALIGN
#endif

#ifdef __CC_WIN32_CE__
    #define STRSAFE_NO_DEPRECATE

    #ifndef WINVER
        #define WINVER __CC_WIN32_CE__
    #endif
    #include <ceconfig.h>
    #include <winsock.h>
    #include <MSWSock.h>
    #include <windows.h>
#else    /*Don't optimize if WINDOWS.H has already been included*/
    #if !defined(NO_WIN32_LEAN_AND_MEAN)   
        #ifndef WIN32_LEAN_AND_MEAN
            #define WIN32_LEAN_AND_MEAN /*Enable LEAN_AND_MEAN support*/  
        #endif
        #ifndef _VCL_LEAN_AND_MEAN
            #define _VCL_LEAN_AND_MEAN /*BCB v1.0 compatible*/
        #endif
    #endif/*NO_WIN32_LEAN_AND_MEAN*/

    #define _CC_WORKER_DISABLE_CONDITION

    #if _CC_MSVC_ >= 1600
        #include <SDKDDKVer.h>
    #endif
    /*
     * we need to include <windows.h> explicitly before <winsock2.h> because
     * the warning 4201 is enabled in <windows.h>
     */
    #include <WinSock2.h>   /* Must be placed before windows.h */
    #include <winsock.h>
    #include <MSWSock.h>
    #include <ws2tcpip.h>
    #include <windows.h>
#endif

#include "../string.h"
#include "../buf.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#if defined(_CC_MSVC_) && _CC_MSVC_ >= 1800
#define strdup _strdup
#endif

#if !defined(__CC_MINGW__)
#define popen(x, y) _popen((x), (y))
#define pclose(x) _pclose(x)
#define fileno _fileno
#endif

#if !defined(__CC_MINGW__)
#define stat64      _stat64
#endif

#define snprintf    _snprintf
#define fileno      _fileno
#define rmdir       _rmdir

/**/
_CC_API_PUBLIC(HMODULE) _cc_load_windows_kernel32(void);
/**/
_CC_API_PUBLIC(void) _cc_unload_windows_kernel32(void);

#ifndef _CC_DISABLED_DUMPER_
#define _CC_DUMPER_SUCCESS_                            0
#define _CC_DUMPER_FAILED_TO_CREATE_DUMP_FILE_         1
#define _CC_DUMPER_FAILED_TO_SAVE_DUMP_FILE_           2
#define _CC_DUMPER_DBGHELP_DLL_NOT_FOUND_              3
#define _CC_DUMPER_DBGHELP_DLL_TOO_OLD_                4

typedef void (*_cc_dumper_callback_t)(byte_t status, pvoid_t dump_exception_info);

/**
 * @brief Install MS DbgHelp.dll Dumpfile Api
 *        The Dumpfile used to export the currently running program
 *
 * @param callback The callback function
 *
 * @return true if successful or false on error.
*/
_CC_API_PUBLIC(bool_t) _cc_install_dumper(_cc_dumper_callback_t callback);

/**
 * @brief uninstall MS DbgHelp.dll Dumpfile Api
 */
_CC_API_PUBLIC(void) _cc_uninstall_dumper(void);

#endif /*ndef _CC_DISABLED_DUMPER_ */

/**/
_CC_API_PUBLIC(const _cc_String_t *) _cc_get_module_file_name(void);
/**/
_CC_API_PUBLIC(size_t) _cc_get_resolve_symbol(tchar_t *buf, size_t length);
/**
 * @brief Multi Byte To Wide Char
 *
 * @param s1 Multi byte buffer
 * @param s1_len The length of the Multi byte
 * @param s2 Wide char buffer
 * @param size Wide char buffer size
 *
 * @return Length of wide char
*/
_CC_API_PUBLIC(int32_t) _cc_a2w(const char_t *s1,
                         int32_t s1_len,
                         wchar_t* s2,
                         int32_t size);
/**
 * @brief Wide Byte To Multi Char
 *
 * @param s1 Wide byte buffer
 * @param s1_len The length of the Wide byte
 * @param s2 Multi char buffer
 * @param size Multi char buffer size
 *
 * @return Length of Multi char
 */
_CC_API_PUBLIC(int32_t) _cc_w2a(const wchar_t *s1,
                         int32_t s1_len,
                         char_t* s2,
                         int32_t size);
/**
 * @brief Get Current Proccess Id
 *
 * @return id
 */
#define _cc_getpid() ((uint32_t)(DWORD)GetCurrentProcessId())


#define _CC_CLOSE_HANDLE(handle)\
do {\
    if (*handle) {\
        CloseHandle(*handle);\
        *handle = nullptr;\
    }\
} while (0)

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_WINDOWS_H_INCLUDED_*/
