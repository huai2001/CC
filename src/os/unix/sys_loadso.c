#include <libcc/loadso.h>
#include <libcc/logger.h>
#include <libcc/cores.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

#ifdef __CC_IPHONEOS__
_CC_API_PUBLIC(bool_t) _cc_is_system_version_at_least(double version);
#endif

_CC_API_PUBLIC(uintptr_t) _cc_load_object(const tchar_t *sofile) {
    void *handle = NULL;
#ifdef __CC_IPHONEOS__
    if (!_cc_is_system_version_at_least(8.0)) {
        _cc_logger(_CC_LOG_LEVEL_ERROR_, "_cc_load_object requires iOS 8+");
        return 0;
    }
#endif
    handle = dlopen(sofile, RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        _cc_logger_error("Failed dlopen %s : %s", sofile, (tchar_t *)dlerror());
        return 0;
    }
    return (uintptr_t)(handle);
}

_CC_API_PUBLIC(uintptr_t) _cc_load_function(uintptr_t handle, const char_t *name) {
    void *symbol = dlsym((void *)handle, name);
    if (!symbol) {
        tchar_t _func_name[256] = {0};

        _func_name[0] = '_';
        _tcsncpy(_func_name + 1, name, _cc_countof(_func_name) - 1);
        _func_name[_cc_countof(_func_name) - 1] = 0;

        symbol = dlsym((void *)handle, _func_name);
        if (!symbol) {
            _cc_logger_error("Failed dlsym(%s): %s", name, (tchar_t *)dlerror());
            return 0;
        }
    }
    return (uintptr_t)(symbol);
}

_CC_API_PUBLIC(void) _cc_unload_object(uintptr_t handle) {
    if (handle) {
        dlclose((pvoid_t)handle);
    }
}
