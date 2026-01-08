#include <libcc/os/unix.h>
#include <libcc/dirent.h>
#include <libcc/logger.h>

#include <execinfo.h>
#include <dlfcn.h>
#include <stdio.h>

#ifndef __CC_APPLE__
#include <sys/utsname.h>
_CC_API_PUBLIC(size_t) _cc_get_device_name(tchar_t *cname, size_t length) {
    struct utsname buffer;
    if (uname(&buffer) == 0) {
        _tcsncpy(cname, buffer.nodename,length);
    } else if (gethostname(cname, length) != 0) {
        _tcsncpy(cname,"localhost.unknown", length);
    }
    cname[length - 1] = 0;
    return _tcslen(cname);
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
    char **symbols = NULL;
    Dl_info info;

    n = backtrace(buffer, _cc_countof(buffer));
    symbols = backtrace_symbols(buffer, n);

    if (symbols == NULL) {
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
_CC_API_PUBLIC(size_t) _cc_get_module_file_name(pvoid_t func, tchar_t *module, size_t length) {
    Dl_info info;
    _cc_assert(module != NULL);
    _cc_assert(length > 0);
    if (module == NULL || length == 0) {
        return 0;
    }

    if (dladdr(func ? func : (pvoid_t)_cc_get_module_file_name, &info)) {
        int i;
        int res = (int)_tcslen(info.dli_fname);
        for (i = res - 1; i > 0; i--) {
            if (info.dli_fname[i] == _CC_SLASH_C_) {
                break;
            }
        }
        if (i >= 1) {
            res = (res - i - 1);
        }
        
        length = ((size_t)res < length) ? (size_t)res : length;
        if (length > 0){
            memcpy(module, info.dli_fname + i + 1, length);
            module[length] = 0;
            return length;
        }
    }

    return 0;
}