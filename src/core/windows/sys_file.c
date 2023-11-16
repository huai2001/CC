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
#include <cc/alloc.h>

#define _GET_HANDLE(x) (HANDLE)(x->fp)
/**/
_CC_API_PUBLIC(bool_t) flock(int fd, int32_t op) {
#ifndef __CC_WIN32_CE__
    HANDLE hdl = (HANDLE)_get_osfhandle(fd);
    DWORD low = 1, high = 0;
    OVERLAPPED offset = {0, 0, 0, 0, NULL};
    if (hdl < 0) {
        return false; /* error in file descriptor */
    }
    /* bug for bug compatible with Unix */
    UnlockFileEx(hdl, 0, low, high, &offset);
    switch (op & ~LOCK_NB) { /* translate to LockFileEx() op */
    case LOCK_EX:            /* exclusive */
        if (LockFileEx(hdl, LOCKFILE_EXCLUSIVE_LOCK + ((op & LOCK_NB) ? LOCKFILE_FAIL_IMMEDIATELY : 0), 0, low, high,
                       &offset))
            return true;
        break;
    case LOCK_SH: /* shared */
        if (LockFileEx(hdl, ((op & LOCK_NB) ? LOCKFILE_FAIL_IMMEDIATELY : 0), 0, low, high, &offset))
            return true;
        break;
    case LOCK_UN:    /* unlock */
        return true; /* always succeeds */
    default:         /* default */
        break;
    }
#endif /*__CC_WIN32_CE__*/
    return false;
}

_CC_API_PRIVATE(int64_t) _cc_win_file_size(_cc_file_t *context) {
    LARGE_INTEGER size;

    if (!context || _GET_HANDLE(context) == INVALID_HANDLE_VALUE) {
        _cc_logger_error(_T("windows_file_size: invalid context/file not opened"));
        return -1;
    }

    if (!GetFileSizeEx(_GET_HANDLE(context), &size)) {
        _cc_logger_error(_T("windows_file_size %s"), _cc_last_error(_cc_last_errno()));
        return -1;
    }

    return size.QuadPart;
}

_CC_API_PRIVATE(bool_t) _cc_win_file_seek(_cc_file_t *context, int64_t offset, int whence) {
    DWORD windowswhence;
    LARGE_INTEGER windowsoffset;

    if (!context || _GET_HANDLE(context) == INVALID_HANDLE_VALUE) {
        _cc_logger_error(_T("windows_file_seek: invalid context/file not opened"));
        return false;
    }

    switch (whence) {
    case _CC_FILE_SEEK_SET_:
        windowswhence = FILE_BEGIN;
        break;
    case _CC_FILE_SEEK_CUR_:
        windowswhence = FILE_CURRENT;
        break;
    case _CC_FILE_SEEK_END_:
        windowswhence = FILE_END;
        break;
    default:
        _cc_logger_error(_T("windows_file_seek: Unknown value for 'whence'"));
        return false;
    }

    windowsoffset.QuadPart = offset;
    if (!SetFilePointerEx(_GET_HANDLE(context), windowsoffset, &windowsoffset, windowswhence)) {
        _cc_logger_error(_T("windows_file_seek %s"), _cc_last_error(_cc_last_errno()));
        return false;
    }
    return true;
}

_CC_API_PRIVATE(size_t) _cc_win_file_read(_cc_file_t *context, pvoid_t ptr, size_t size, size_t maxnum) {
    size_t total_need;
    DWORD byte_read;

    total_need = size * maxnum;

    if (!context || _GET_HANDLE(context) == INVALID_HANDLE_VALUE || !total_need) {
        return 0;
    }

    if (!ReadFile(_GET_HANDLE(context), ptr, (DWORD)total_need, &byte_read, NULL)) {
        return 0;
    }

    return byte_read;
}

_CC_API_PRIVATE(size_t) _cc_win_file_write(_cc_file_t *context, const pvoid_t ptr, size_t size, size_t num) {
    size_t total_bytes = size * num;
    DWORD byte_written = 0;

    if (!context || _GET_HANDLE(context) == INVALID_HANDLE_VALUE || total_bytes <= 0 || !size) {
        return 0;
    }

    /* if in append mode, we must go to the EOF before write */
    if (context->append) {
        if (SetFilePointer(_GET_HANDLE(context), 0L, NULL, FILE_END) == INVALID_SET_FILE_POINTER) {
            return 0;
        }
    }

    if (!WriteFile(_GET_HANDLE(context), ptr, (DWORD)total_bytes, &byte_written, NULL)) {
        return 0;
    }

    return ((size_t)byte_written / size);
}

_CC_API_PRIVATE(bool_t) _cc_win_file_close(_cc_file_t *context) {
    if (!context || _GET_HANDLE(context) == NULL) {
        _cc_logger_error(_T("_cc_win_file_close: invalid context/file not closed"));
        return false;
    }

    if (_GET_HANDLE(context)) {
        CloseHandle(_GET_HANDLE(context));
        _GET_HANDLE(context) = INVALID_HANDLE_VALUE;
    }

    _cc_free(context);

    return true;
}
/**/
_CC_API_PUBLIC(bool_t) _cc_sys_open_file(_cc_file_t *f, const tchar_t *filename, const tchar_t *mode) {
    HANDLE h;
    DWORD r_right = 0, w_right = 0;
    const tchar_t *p = mode;

    /* "r" = reading, file must exist */
    /* "w" = writing, truncate existing, file may not exist */
    /* "r+"= reading or writing, file must exist            */
    /* "a" = writing, append file may not exist             */
    /* "a+"= append + read, file may not exist              */
    /* "w+" = read, write, truncate. file may not exist    */

    /*r,w,a,+*/
    int fmode[4] = {0, 0, 0, 0};
    while (*p) {
        switch (*p) {
        case _T('r'):
            fmode[0] = OPEN_EXISTING;
            break;
        case _T('w'):
            fmode[1] = CREATE_ALWAYS;
            break;
        case _T('a'):
            fmode[2] = OPEN_ALWAYS;
            break;
        case _T('+'):
            fmode[3] = 1;
            break;
        }
        p++;
    }

    if (fmode[0] || fmode[3]) {
        r_right = GENERIC_READ;
    }

    if (fmode[1] || fmode[2] || fmode[3]) {
        w_right = GENERIC_WRITE;
    }

    /* inconsistent mode */
    if (!r_right && !w_right) {
        return false;
    }

    h = CreateFile(filename, (r_right | w_right), (w_right ? 0 : FILE_SHARE_READ), NULL,
                   (fmode[0] | fmode[1] | fmode[2]), FILE_ATTRIBUTE_NORMAL, NULL);

    if (h == INVALID_HANDLE_VALUE) {
        return false;
    }

    f->append = fmode[2] != 0;
    f->fp = h;

    f->read = _cc_win_file_read;
    f->write = _cc_win_file_write;
    f->size = _cc_win_file_size;
    f->seek = _cc_win_file_seek;
    f->close = _cc_win_file_close;

    return true;
}
