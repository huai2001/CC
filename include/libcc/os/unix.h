#ifndef _C_CC_SYS_UNIX_HEAD_FILE_
#define _C_CC_SYS_UNIX_HEAD_FILE_

#include "../string.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define _CC_HAVE_SYSCONF_       1
#define _CC_HAVE_SYSCTLBYNAME_  1

#ifndef _access
#define _access access
#endif

#ifndef _unlink
#define _unlink unlink
#endif

/***/
#define _cc_getpid() ((uint32_t)getpid())

/**/
_CC_API_PUBLIC(int32_t) _cc_a2w(const char_t *s1,int32_t s1_len,wchar_t* s2,int32_t size);
/**/
_CC_API_PUBLIC(int32_t) _cc_w2a(const wchar_t *s1,int32_t s1_len,char_t* s2,int32_t size);

#ifndef _CC_DISABLED_DUMPER_

#define _CC_DUMPER_SUCCESS_                            0
#define _CC_DUMPER_FAILED_TO_CREATE_DUMP_FILE_         1
#define _CC_DUMPER_FAILED_TO_SAVE_DUMP_FILE_           2
#define _CC_DUMPER_DBGHELP_DLL_NOT_FOUND_              3
#define _CC_DUMPER_DBGHELP_DLL_TOO_OLD_                4

typedef void (*_cc_dumper_callback_t)(byte_t status, pvoid_t dump_exception_info);

/**/
#define _cc_install_dumper(expr) ((void)0)
/**/
#define _cc_uninstall_dumper() ((void)0)

/**/
_CC_API_PUBLIC(size_t) _cc_get_resolve_symbol(tchar_t *buf, size_t length);
/**/
_CC_API_PUBLIC(size_t) _cc_get_module_file_name(pvoid_t func, tchar_t *module, size_t length);
/**/
_CC_API_PUBLIC(size_t) _cc_get_device_name(tchar_t *cname, size_t length);
/**/
_CC_API_PUBLIC(void) _cc_get_os_version(uint32_t *major, uint32_t *minor, uint32_t *build);

#endif /*ndef _CC_DISABLED_DUMPER_ */

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_SYS_UNIX_HEAD_FILE_*/




