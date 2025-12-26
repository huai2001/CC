#include <libcc/os.h>
#include <libcc/loadso.h>
#include <libcc/logger.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

#ifdef __CC_IPHONEOS__
_CC_API_PUBLIC(bool_t) _cc_is_system_version_at_least(double version);
#endif

_CC_API_PUBLIC(pvoid_t) _cc_load_object(const tchar_t *sofile) {
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

_CC_API_PUBLIC(pvoid_t) _cc_load_function(pvoid_t handle, const char_t *name) {
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

_CC_API_PUBLIC(void) _cc_unload_object(pvoid_t handle) {
    if (handle != NULL) {
        dlclose(handle);
    }
}
