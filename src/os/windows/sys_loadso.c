#include <libcc/os.h>
#include <libcc/loadso.h>
#include <libcc/logger.h>

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
        _cc_logger_error("Failed loading: %s(%d) %s", sofile, e, _cc_last_error(e));
    }
#ifndef _WIN32_WCE
    SetErrorMode(em);
#endif
    return (void *)handle;
}

/**/
_CC_API_PUBLIC(pvoid_t) _cc_load_function(pvoid_t handle, const char_t *name) {
    if (handle != NULL) {
        return (pvoid_t)GetProcAddress((HMODULE)handle, name);
    }
    return NULL;
}

/**/
_CC_API_PUBLIC(void) _cc_unload_object(pvoid_t handle) {
    if (handle != NULL) {
        FreeLibrary((HMODULE)handle);
    }
}