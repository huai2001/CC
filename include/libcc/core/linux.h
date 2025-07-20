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
#ifndef _C_CC_SYS_LINUX_HEAD_FILE_
#define _C_CC_SYS_LINUX_HEAD_FILE_

#include "../buf.h"
#include <string.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define _CC_HAVE_SYSCONF_ 1
/**
 * @brief Get Current Proccess Id
 *
 * @return id
 */
#define _cc_getpid() ((int32_t)getpid())

#ifndef _CC_DISABLED_DUMPER_
    
#define _CC_DUMPER_SUCCESS_                            0
#define _CC_DUMPER_FAILED_TO_CREATE_DUMP_FILE_         1
#define _CC_DUMPER_FAILED_TO_SAVE_DUMP_FILE_           2
#define _CC_DUMPER_DBGHELP_DLL_NOT_FOUND_              3
#define _CC_DUMPER_DBGHELP_DLL_TOO_OLD_                4

typedef void (*_cc_dumper_callback_t)(byte_t status, pvoid_t dump_exception_info);

/**
 * @brief Compatible with WINDOWS mode
 */
#define _cc_install_dumper(expr) ((void)0)

/**
 * @brief Compatible with WINDOWS mode
 */
#define _cc_uninstall_dumper() ((void)0)

_CC_API_PUBLIC(tchar_t**) _cc_get_stack_trace(int *nptr);

#ifndef _access
#define _access access
#endif

#endif /*ndef _CC_DISABLED_DUMPER_ */

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_SYS_LINUX_HEAD_FILE_*/




