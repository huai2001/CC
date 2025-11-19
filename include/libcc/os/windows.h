#ifndef _C_CC_WINDOWS_H_INCLUDED_
#define _C_CC_WINDOWS_H_INCLUDED_

#include "../types.h"

#if !defined(_WIN64) && !defined(_WIN32)
#error Unsupported OS
#endif

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
            #define WIN32_LEAN_AND_MEAN     /*Enable LEAN_AND_MEAN support*/  
        #endif
        #ifndef _VCL_LEAN_AND_MEAN
            #define _VCL_LEAN_AND_MEAN      /*BCB v1.0 compatible*/
        #endif
    #endif/*NO_WIN32_LEAN_AND_MEAN*/

    #define _CC_WORKER_DISABLE_CONDITION

    #if __CC_MSVC__ >= 1600
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

/* Protections */
#define PROT_NONE           0x00            /* no permissions */
#define PROT_READ           0x01            /* pages can be read */
#define PROT_WRITE          0x02            /* pages can be written */
#define PROT_EXEC           0x04            /* pages can be executed */

/* Sharing type and options */
#define MAP_SHARED          0x0001          /* share changes */
#define MAP_PRIVATE         0x0002          /* changes are private */
#define MAP_COPY            MAP_PRIVATE     /* Obsolete */
#define MAP_FIXED           0x0010          /* map addr must be exactly as requested */
#define MAP_RENAME          0x0020          /* Sun: rename private pages to file */
#define MAP_NORESERVE       0x0040          /* Sun: don't reserve needed swap area */
#define MAP_INHERIT         0x0080          /* region is retained after exec */
#define MAP_NOEXTEND        0x0100          /* for MAP_FILE, don't change file size */
#define MAP_HASSEMAPHORE    0x0200          /* region may contain semaphores */
#define MAP_STACK           0x0400          /* region grows down, like a stack */

/* Error returned from mmap() */
#define MAP_FAILED          ((void *)-1)

/* Flags to msync */
#define MS_ASYNC            0x01            /* perform asynchronous writes */
#define MS_SYNC             0x02            /* perform synchronous writes */
#define MS_INVALIDATE       0x04            /* invalidate cached data */

/* File modes for 'open' not defined in MinGW32  (not used by mmap) */
#ifndef S_IWGRP
#define S_IWGRP             0
#define S_IRGRP             0
#define S_IROTH             0
#endif

#define LOCK_SH             1
#define LOCK_EX             2
#define LOCK_NB             4
#define LOCK_UN             8

#if defined(__CC_MSVC__) && __CC_MSVC__ >= 1800
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

#define _cc_close_handle(H)\
do {\
    if (*H) {\
        CloseHandle(*H);\
        *H = nullptr;\
    }\
} while (0)

/**/
#define _cc_getpid() ((uint32_t)(DWORD)GetCurrentProcessId())

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* {{{ mmap */
/**/
_CC_API_PUBLIC(pvoid_t) mmap(pvoid_t addr, unsigned int len, int prot, int flags, int fd, unsigned int offset);
/**/
_CC_API_PUBLIC(int) munmap(pvoid_t addr, int len);
/**/
_CC_API_PUBLIC(int) msync(char *addr, int len, int flags);
/* }}} */

/* {{{ file lock */
_CC_API_PUBLIC(bool_t) flock(int, int32_t);
/* }}} */

/**/
_CC_API_PUBLIC(HMODULE) _cc_load_windows_kernel32(void);
/**/
_CC_API_PUBLIC(void) _cc_unload_windows_kernel32(void);

/* {{{ dumper */
#ifndef _CC_DISABLED_DUMPER_

#define _CC_DUMPER_SUCCESS_                            0
#define _CC_DUMPER_FAILED_TO_CREATE_DUMP_FILE_         1
#define _CC_DUMPER_FAILED_TO_SAVE_DUMP_FILE_           2
#define _CC_DUMPER_DBGHELP_DLL_NOT_FOUND_              3
#define _CC_DUMPER_DBGHELP_DLL_TOO_OLD_                4

typedef void (*_cc_dumper_callback_t)(byte_t status, pvoid_t dump_exception_info);

/**/
_CC_API_PUBLIC(bool_t) _cc_install_dumper(_cc_dumper_callback_t callback);
/**/
_CC_API_PUBLIC(void) _cc_uninstall_dumper(void);

#endif /*ndef _CC_DISABLED_DUMPER_ */

/* }}} */

/**/
_CC_API_PUBLIC(const _cc_string_t *) _cc_get_module_file_name(void);
/**/
_CC_API_PUBLIC(size_t) _cc_get_resolve_symbol(tchar_t *buf, size_t length);
/**/
_CC_API_PUBLIC(size_t) _cc_get_device_name(tchar_t *cname, size_t length);
/**/
_CC_API_PUBLIC(void) _cc_get_os_version(uint32_t *major, uint32_t *minor, uint32_t *build);
/**/
_CC_API_PUBLIC(int32_t) _cc_a2w(const char_t *s1, int32_t s1_len, wchar_t* s2, int32_t size);
/**/
_CC_API_PUBLIC(int32_t) _cc_w2a(const wchar_t *s1, int32_t s1_len, char_t* s2, int32_t size);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_WINDOWS_H_INCLUDED_*/
