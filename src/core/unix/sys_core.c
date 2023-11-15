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

/* System dependent filesystem routines */
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

#include <cc/alloc.h>
#include <cc/core.h>
#include <cc/dirent.h>
#include <unistd.h>

#if defined(__CC_FREEBSD__) || defined(__CC_OPENBSD__)
#include <sys/sysctl.h>
#endif


_CC_API_PUBLIC(int32_t) _cc_a2w(const char_t *s1, int32_t s1_len, wchar_t *s2, int32_t size) {
    return _cc_utf8_to_utf16((const uint8_t *)s1, (const uint8_t *)(s1 + s1_len), (uint16_t *)s2,
                             (uint16_t *)(s2 + size), false);
}

_CC_API_PUBLIC(int32_t) _cc_w2a(const wchar_t *s1, int32_t s1_len, char_t *s2, int32_t size) {
    return _cc_utf16_to_utf8((const uint16_t *)s1, (const uint16_t *)(s1 + s1_len), (uint8_t *)s2,
                             (uint8_t *)(s2 + size), false);
}

_CC_API_PRIVATE(int32_t) _sym_link(tchar_t *cwd, int32_t maxlen);

/**/
_CC_API_PUBLIC(int32_t) _cc_set_clipboard_text(const tchar_t *str) {
    return 1;
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_clipboard_text(tchar_t *str, int32_t len) {
    return 1;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_has_clipboard_text(void) {
    return true;
}

_CC_API_PUBLIC(void) _cc_set_last_errno(int32_t _errno) {
    errno = _errno;
}

_CC_API_PUBLIC(int32_t) _cc_last_errno(void) {
    return errno;
}

_CC_API_PUBLIC(tchar_t*) _cc_last_error(int32_t _errno) {
    return strerror(_errno);
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_computer_name(tchar_t *name, int32_t maxlen) {
    if (gethostname(name, maxlen) == 0) {
        return (int32_t)strlen(name);
    }
    return 0;
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_current_directory(tchar_t *cwd, int32_t maxlen) {
    if (getcwd(cwd, maxlen) != NULL) {
        return (int32_t)strlen(cwd);
    }
    return 0;
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_current_file(tchar_t *cwd, int32_t maxlen) {
    int32_t rc = _sym_link(cwd, maxlen);
    if (rc <= 0) {
        return 0;
    }
    return rc;
}
/**/
_CC_API_PUBLIC(int32_t) _cc_get_module_file_name(tchar_t *cwd, int32_t maxlen) {
    tchar_t path[_CC_MAX_PATH_];
    int32_t len = 0;
    int32_t i = 0;
    int32_t rc = (int32_t)_sym_link(path, _CC_MAX_PATH_);

    if (rc <= 0) {
        return 0;
    }

    for (i = rc - 1; i >= 0; i--) {
        if (path[i] == _CC_T_PATH_SEP_C_) {
            i++;
            break;
        }
    }

    if (i > 0 && (rc - i) < maxlen) {
        for (; i < rc; i++) {
            cwd[len++] = path[i];
        }
        cwd[len] = 0;
        return len;
    }
    return -1;
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_module_document_directory(tchar_t *cwd, int32_t maxlen) {
    /*
    char_t *home = getenv("HOME");
    if (home) {
        _sntprintf(cwd, maxlen, _T("%s/documents"), home);
    }*/
    return _cc_get_module_directory(_T("documents"), cwd, maxlen);
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_module_cache_directory(tchar_t *cwd, int32_t maxlen) {
    /*
    char_t *home = getenv("HOME");
    if (home) {
        _sntprintf(cwd, maxlen, _T("%s/cache"), home);
    }*/

    return _cc_get_module_directory(_T("cache"), cwd, maxlen);
}

/* QNX's /proc/self/exefile is a text file and not a symlink. */
#if !defined(__CC_QNXNTO__)
_CC_API_PRIVATE(int32_t) readSymLink(const char *path, tchar_t *cwd, int32_t maxlen) {
    int32_t rc = (int32_t)readlink(path, cwd, maxlen);
    /* not a symlink, i/o error, etc. */
    if (rc == -1) {
        return -1;
    } else if (rc < maxlen) {
        /* readlink doesn't null-terminate. */
        cwd[rc] = '\0';
    } else {
        cwd[maxlen - 1] = '\0';
    }
    return rc;
}
#endif

_CC_API_PRIVATE(int32_t) _sym_link(tchar_t *cwd, int32_t maxlen) {
    int32_t rc = 0;
#if defined(__CC_FREEBSD__)
    rc = maxlen;
    const int mib[] = {CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1};
    if (sysctl(mib, _cc_countof(mib), cwd, (size_t *)&rc, NULL, 0) != -1) {
        if (!cwd) {
            _cc_logger_error("failed : sysctl! ");
            return 0;
        }
        return rc;
    }
#endif

#if defined(__CC_OPENBSD__)
    char **retvalargs;
    const int mib[] = {CTL_KERN, KERN_PROC_ARGS, getpid(), KERN_PROC_ARGV};
    if (sysctl(mib, 4, NULL, &rc, NULL, 0) != -1) {
        retvalargs = (char **)_cc_malloc(rc);
        sysctl(mib, 4, retvalargs, &rc, NULL, 0);
        realpath(retvalargs[0], cwd);
        _cc_free(retvalargs);

        return (int32_t)strlen(cwd);
    }
#endif

#if defined(__CC_SOLARIS__)
    char *retval = getexecname();
    if ((retval != NULL) && (retval[0] == '/')) { /* must be absolute path... */
        _tcsncpy(cwd, retval, maxlen);
        cwd[maxlen - 1] = 0;
        return (int32_t)strlen(cwd);
    }
#endif

    /* is a Linux-style /proc filesystem available? */
    if (access("/proc", F_OK) == 0) {
        /* !!! FIXME: after 2.0.6 ships, let's delete this code and just
                      use the /proc/%llu version. There's no reason to have
                      two copies of this plus all the #ifdefs. --ryan. */
#if defined(__CC_FREEBSD__)
        rc = readSymLink("/proc/curproc/file", cwd, maxlen);
#elif defined(__CC_NETBSD__)
        rc = readSymLink("/proc/curproc/exe", cwd, maxlen);
#else
        rc = readSymLink("/proc/self/exe", cwd, maxlen); /* linux. */
        if (rc <= 0) {
            /* older kernels don't have /proc/self ... try PID version... */
            char path[64];
            snprintf(path, _cc_countof(path), "/proc/%llu/exe", (unsigned long long)getpid());

            rc = readSymLink(path, cwd, maxlen);
        }
#endif
        if (rc <= 0) {
            return 0;
        }
    }
    return rc;
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_executable_directory(tchar_t *cwd, int32_t maxlen) {
    return _sym_link(cwd, maxlen);
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_module_directory(const tchar_t *module, tchar_t *cwd, int32_t maxlen) {
    int32_t i = 0;
    int32_t rc = _sym_link(cwd, maxlen);
    if (rc <= 0) {
        return 0;
    }

    for (i = rc - 1; i >= 0; i--) {
        if (cwd[i] == _CC_T_PATH_SEP_C_) {
            /* chop off filename. */
            cwd[i++] = 0;
            break;
        }
    }

    if (module && i < maxlen) {
        tchar_t *p = (tchar_t *)module;
        cwd[i - 1] = _CC_T_PATH_SEP_C_;
        while (i < maxlen) {
            cwd[i++] = *p++;
            if (*p == 0) {
                break;
            }
        }
        cwd[i++] = 0;
    }
    return -1;

}

/**/
_CC_API_PUBLIC(bool_t) _cc_set_current_directory(tchar_t *cwd) {
    if (cwd == NULL) {
        return false;
    }
    return chdir(cwd) == 0;
}
