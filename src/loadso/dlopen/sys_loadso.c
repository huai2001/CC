/*
 * Copyright .Qiu<huai2011@163.com>. and other libCC contributors.
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
#include <cc/core.h>
#include <cc/loadso.h>
#include <cc/logger.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

#ifdef __CC_IPHONEOS__
bool_t _cc_is_system_version_at_least(double version);
#endif

pvoid_t _cc_load_object(const tchar_t *sofile) {
    pvoid_t handle = NULL;
#ifdef __CC_IPHONEOS__
    if (!_cc_is_system_version_at_least(8.0)) {
        _cc_logger_error(_T("_cc_load_object requires iOS 8+"));
        return NULL;
    }
#endif
    handle = dlopen(sofile, RTLD_NOW | RTLD_LOCAL);
    if (handle == NULL) {
        _cc_logger_error(_T("Failed dlopen %s : %s"), sofile, (tchar_t *)dlerror());
    }

    return (handle);
}

pvoid_t _cc_load_function(pvoid_t handle, const char_t *name) {
    pvoid_t symbol = dlsym(handle, name);
    if (symbol == NULL) {
        tchar_t _func_name[256] = {0};

        _func_name[0] = '_';
        _tcsncpy(_func_name + 1, name, _cc_countof(_func_name) - 1);
        _func_name[_cc_countof(_func_name) - 1] = 0;

        symbol = dlsym(handle, _func_name);
        if (symbol == NULL) {
            _cc_logger_error(_T("Failed dlsym(%s): %s"), name, (tchar_t *)dlerror());
        }
    }
    return (symbol);
}

void _cc_unload_object(pvoid_t handle) {
    if (handle != NULL) {
        dlclose(handle);
    }
}
