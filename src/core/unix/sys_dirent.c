/*
 * Copyright libcc.cn@gmail.com. and other libcc contributors.
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

#include <libcc/alloc.h>
#include <libcc/dirent.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>

#ifdef __CC_BSD__
#include <sys/sysctl.h>
#endif

/* QNX's /proc/self/exefile is a text file and not a symlink. */
#if !defined(__CC_QNXNTO__)
_CC_API_PRIVATE(int32_t) readSymLink(const char *path, tchar_t *cwd, int32_t maxlen) {
    int32_t rc = (int32_t)readlink(path, cwd, maxlen);
    /* not a symlink, i/o error, etc. */
    if (rc == -1) {
        return -1;
    } else if (rc < maxlen) {
        /* readlink doesn't nullptr-terminate. */
        cwd[rc] = '\0';
    } else {
        cwd[maxlen - 1] = '\0';
    }
    return rc;
}
#endif

_CC_API_PRIVATE(size_t) _sym_link(tchar_t *cwd, size_t maxlen) {
    size_t rc = 0;
#if defined(__CC_FREEBSD__)
    rc = maxlen;
    const int mib[] = {CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1};
    if (sysctl(mib, _cc_countof(mib), cwd, &rc, nullptr, 0) != -1) {
        if (!cwd) {
            _cc_logger_error("failed : sysctl! ");
            return 0;
        }
        return rc;
    }
#endif

#if defined(__CC_OPENBSD__)
    char **args;
    const int mib[] = {CTL_KERN, KERN_PROC_ARGS, getpid(), KERN_PROC_ARGV};
    if (sysctl(mib, 4, nullptr, &rc, nullptr, 0) != -1) {
        args = (char **)_cc_malloc(rc);
        sysctl(mib, 4, args, &rc, nullptr, 0);
        realpath(args[0], cwd);
        _cc_free(args);

        rc = (size_t)strlen(cwd);

        return rc;
    }
#endif

    /* is a Linux-style /proc filesystem available? */
    if (rc <= 0 && (access("/proc", F_OK) == 0)) {
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
            rc = snprintf(path, _cc_countof(path), "/proc/%llu/exe", (unsigned long long)getpid());
            if (rc > 0 && rc < sizeof(path)) {
                rc = readSymLink(path, cwd, maxlen);
            }
            
        }
#endif
    }

    #if defined(__CC_SOLARIS__)
    if (rc <= 0) {
        char *path = getexecname();
        if ((path != nullptr) && (path[0] == _CC_SLASH_C_)) { /* must be absolute path... */
            _tcsncpy(cwd, path, maxlen);
            cwd[maxlen - 1] = 0;
            rc = (size_t)strlen(cwd);
        }
    }

#endif
    return rc;
}

_CC_API_PUBLIC(size_t) _cc_get_executable_path(tchar_t *path, size_t len) {
    return _sym_link(path, len);
}

_CC_API_PUBLIC(size_t) _cc_get_base_path(tchar_t *path, size_t len) {
    size_t i;
    size_t rc = _sym_link(path, len);
    if (rc <= 0) {
        return 0;
    }
    for (i = rc - 1; i > 0; i--) {
        if (path[i] == _CC_SLASH_C_) {
            break;
        }
    }

    _cc_assert(i > 0);  // Should have been an absolute path.
    path[i] = '\0'; // chop off filename.

    return i;

}

_CC_API_PUBLIC(size_t) _cc_get_folder(_cc_folder_t folder, tchar_t *path, size_t len) {
    const tchar_t *param = nullptr;
    struct passwd *pw = getpwuid(getuid());

    switch(folder) {
    case _CC_FOLDER_HOME_:
        break;
    case _CC_FOLDER_DESKTOP_:
        param = _T("Desktop");
        break;

    case _CC_FOLDER_DOCUMENTS_:
        param = _T("Document");
        break;

    case _CC_FOLDER_DOWNLOADS_:
        param = _T("Download");
        break;

    case _CC_FOLDER_MUSIC_:
        param = _T("Music");
        break;

    case _CC_FOLDER_PICTURES_:
        param = _T("Pictures");
        break;

    case _CC_FOLDER_PUBLICSHARE_:
        param = _T("Publicshare");
        break;

    case _CC_FOLDER_SAVEDGAMES_:
        param = _T("SavedGames");
        break;

    case _CC_FOLDER_SCREENSHOTS_:
        param = _T("Screenshots");
        break;

    case _CC_FOLDER_TEMPLATES_:
        param = _T("Templates");
        break;

    case _CC_FOLDER_VIDEOS_:
        param = _T("Videos");
        break;
    default:
        _cc_logger_error(_T("Invalid _cc_folder_: %d"), (int) folder);
        return 0;
    }

    if (param) {
        return _sntprintf(path, len, "%s/%s", pw->pw_dir, param);
    } else {
        _tcsncpy(path, pw->pw_dir, len);
        path[len - 1] = 0;
        return _tcslen(path);
    }
    return 0;
}
