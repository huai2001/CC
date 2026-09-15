#include <libcc/loadso.h>
#include <libcc/logger.h>

/**/
_CC_API_PUBLIC(uintptr_t) _cc_load_object(const tchar_t *sofile) {
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
        _cc_logger_error("Failed loading: %s(%d) %s", sofile, e, _cc_last_error(e));
    }
#ifndef _WIN32_WCE
    SetErrorMode(em);
#endif
    return (uintptr_t)handle;
}

/**/
_CC_API_PUBLIC(uintptr_t) _cc_load_function(uintptr_t handle, const char_t *name) {
    if (handle != NULL) {
        uintptr_t symbol = (uintptr_t)GetProcAddress((HMODULE)handle, name);
        if (symbol == 0) {
            _cc_logger_error("Failed dlsym(%s): %s", name, (tchar_t *)dlerror());
        }
        return symbol;
    }
    return NULL;
}

/**/
_CC_API_PUBLIC(void) _cc_unload_object(uintptr_t handle) {
    if (handle) {
        FreeLibrary((HMODULE)handle);
    }
}