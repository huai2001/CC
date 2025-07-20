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
#include <execinfo.h>
#include <stdio.h>
#include <libcc/core/unix.h>
#include <libcc/dirent.h>
#include <libcc/logger.h>

_CC_API_PUBLIC(bool_t) _cc_isdir(const tchar_t *dir_path) {
    struct stat st;
    int res = stat(dir_path, &st);

    if (res < 0) {
        return false;
    }

    if (S_ISDIR(st.st_mode)) {
        return true;
    }

    return false;
}

_CC_API_PUBLIC(tchar_t**) _cc_get_stack_trace(int *nptr) {
    int n;
    pvoid_t buffer[64];
    char **symbols;
    
    n = backtrace(buffer, _cc_countof(buffer));
    if (nptr) {
        *nptr = n;
    }
    return backtrace_symbols(buffer, n);
}