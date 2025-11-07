#include <libcc/os/unix.h>
#include <libcc/dirent.h>
#include <libcc/logger.h>

#include <execinfo.h>
#include <dlfcn.h>
#include <stdio.h>

#ifndef __CC_APPLE__
_CC_API_PUBLIC(size_t) _cc_get_device_name(tchar_t *cname, size_t length) {
    cname[0] = 0;
    return 0;
}
#endif

/**/
_CC_API_PUBLIC(void) _cc_get_os_version(uint32_t *major, uint32_t *minor, uint32_t *build) {
    *major = 0;
    *minor = 0;
    *build = 0;
}

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
    int n, i;
    size_t r = 0;
    pvoid_t buffer[128];
    char **symbols = nullptr;
    Dl_info info;

    n = backtrace(buffer, _cc_countof(buffer));
    symbols = backtrace_symbols(buffer, n);

    if (symbols == nullptr) {
        return 0;
    }

    for (i = 1; i < n; i++) {
        size_t fmt_length = 0;
        size_t remaining = length - r;
        if (dladdr(symbols[i], &info)) {
            fmt_length = _sntprintf(buf + r, remaining, _T("%s (%s)\n"), info.dli_sname, info.dli_fname);
        } else if (symbols[i]) {
            fmt_length = _sntprintf(buf + r, remaining, _T("%s\n"), symbols[i]);
        } else {
            if (remaining < 7) {
                break;
            }
            fmt_length = _sntprintf(buf + r, remaining, _T("(null)\n"));
        }

        if (fmt_length <= 0 || fmt_length >= remaining) {
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
_CC_API_PUBLIC(const _cc_string_t *) _cc_get_module_file_name(void) {
    static tchar_t dl[64];
    static _cc_string_t path = {0, dl};
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
            path.ptr = dl;
            path.length = (length - i) & 63;
            memmove(dl,dl + i + 1, path.length);
            dl[path.length] = 0;
        }
    }

    return &path;
}
