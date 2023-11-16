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

/**/
_CC_API_PUBLIC(pvoid_t) _cc_load_object(const tchar_t *sofile) {
    HINSTANCE handle;
    /* Prevent ugly popups from killing our app */
#ifndef _WIN32_WCE
    UINT em = SetErrorMode(SEM_FAILCRITICALERRORS);
#endif

#ifdef __CC_WINRT__
    /* WinRT only publically supports LoadPackagedLibrary() for loading .dll
       files.  LoadLibrary() is a private API, and not available for apps
       (that can be published to MS' Windows Store.)
    */
    handle = LoadPackagedLibrary(sofile, 0);
#else
    handle = LoadLibraryEx(sofile, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (handle == NULL) {
        handle = LoadLibraryEx(sofile, NULL, 0);
    }
#endif

    if (handle == NULL) {
        int32_t e = _cc_last_errno();
        _cc_logger_error(_T("Failed loading: %s(%d) %s"), sofile, e, _cc_last_error(e));
    }

#ifndef _WIN32_WCE
    SetErrorMode(em);
#endif

    return (void *)handle;
}

/**/
_CC_API_PUBLIC(pvoid_t) _cc_load_function(pvoid_t handle, const char_t *name) {
    pvoid_t symbol = GetProcAddress((HMODULE)handle, name);
    if (symbol == NULL) {
        int32_t e = _cc_last_errno();
#ifdef _CC_UNICODE_
        wchar_t buf[128];
        _cc_a2w(name, strlen(name), buf, _cc_countof(buf));
        _cc_logger_error(_T("GetProcAddress(%s) error:%d %s"), buf, e, _cc_last_error(e));
#endif
        _cc_logger_error(_T("GetProcAddress(%s) error:%d %s"), name, e, _cc_last_error(e));

    }
    return symbol;
}

/**/
_CC_API_PUBLIC(void) _cc_unload_object(pvoid_t handle) {
    if (handle != NULL) {
        FreeLibrary((HMODULE)handle);
    }
}