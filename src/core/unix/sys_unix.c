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
#include <dlfcn.h>
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

_CC_API_PUBLIC(size_t) _cc_get_resolve_symbol(tchar_t *buf, size_t length) {
    int n,i;
    size_t r;
    pvoid_t buffer[64];
    char **symbols;
    
    n = backtrace(buffer, _cc_countof(buffer));
    symbols = backtrace_symbols(buffer, n);
    
    if (symbols == nullptr) {
        return 0;
    }

    for (r = 0, i = 1; i < n; i++) {
        size_t fmt_length = _sntprintf(buf + r, length - r, _T("{%s},"), symbols[i]);
        if (fmt_length <= 0 || fmt_length > (length - r)) {
            break;
        }
        r += fmt_length;
    }
    
    if (r > 0) {
        buf[r - 1] = 0;
    }
    free(symbols);
    return r;
}

/**/
_CC_API_PUBLIC(const _cc_String_t *) _cc_get_module_file_name(void) {
    static TCHAR dl[64];
    static _cc_String_t path = {0, dl};
    if (path.length == 0) {
        Dl_info info;
        size_t length = 0, i;
        if (!dladdr((void*)_cc_get_module_file_name, &info)) {
            return &path;
        }
        for (i = length - 1; i > 0; i--) {
            if (dl[i] == _CC_SLASH_C_) {
                break;
            }
        }
        if (i > 0) {
            path.data = dl;
            path.length = length - i;
            memmove(dl,dl + i + 1, length - i);
            dl[length - 1] = 0;
        }
    }

    return &path;
}