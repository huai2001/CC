/*
 * Copyright libcc.cn@gmail.com. and other libCC contributors.
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
#include <libcc/core.h>
#include <libcc/loadso.h>
#include <libcc/logger.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

#ifdef __CC_IPHONEOS__
_CC_API_PUBLIC(bool_t) _cc_is_system_version_at_least(double version);
#endif

_CC_API_PUBLIC(pvoid_t) _cc_load_object(const tchar_t *sofile) {
    pvoid_t handle = nullptr;
#ifdef __CC_IPHONEOS__
    if (!_cc_is_system_version_at_least(8.0)) {
        _cc_logger_error(_T("_cc_load_object requires iOS 8+"));
        return nullptr;
    }
#endif
    handle = dlopen(sofile, RTLD_NOW | RTLD_LOCAL);
    if (handle == nullptr) {
        _cc_logger_error(_T("Failed dlopen %s : %s"), sofile, (tchar_t *)dlerror());
    }

    return (handle);
}

_CC_API_PUBLIC(pvoid_t) _cc_load_function(pvoid_t handle, const char_t *name) {
    pvoid_t symbol = dlsym(handle, name);
    if (symbol == nullptr) {
        tchar_t _func_name[256] = {0};

        _func_name[0] = '_';
        _tcsncpy(_func_name + 1, name, _cc_countof(_func_name) - 1);
        _func_name[_cc_countof(_func_name) - 1] = 0;

        symbol = dlsym(handle, _func_name);
        if (symbol == nullptr) {
            _cc_logger_error(_T("Failed dlsym(%s): %s"), name, (tchar_t *)dlerror());
        }
    }
    return (symbol);
}

_CC_API_PUBLIC(void) _cc_unload_object(pvoid_t handle) {
    if (handle != nullptr) {
        dlclose(handle);
    }
}
