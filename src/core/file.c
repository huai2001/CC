/*
 * Copyright libcc.cn@gmail.com. and other libCC contributors.
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
#include <libcc/core.h>
#include <libcc/logger.h>

/**/
_CC_API_PUBLIC(bool_t) _cc_sys_open_file(_cc_file_t *f, const tchar_t *filename, const tchar_t *mode);

#ifndef __CC_WINDOWS__
#include <errno.h>
#include <sys/errno.h>

#define _GET_HANDLE(x) (FILE *)(x->fp)

_CC_API_PRIVATE(int) fd_datasync(int fd) {
    int result = 0;
#if defined(__CC_APPLE__)  // Apple doesn't have fdatasync (rather, the symbol exists as an incompatible system call).
    result = fcntl(fd, F_FULLFSYNC);
#endif
    return result;
}
_CC_API_PRIVATE(bool_t) io_flush(_cc_file_t *context) {
    int result;
    int fd;

    if (fflush(_GET_HANDLE(context)) != 0) {
        if (errno == EAGAIN) {
            return false;
        } else {
            _cc_logger_error(_T("Error flushing datastream: %s"), strerror(errno));
            return false;
        }
    }
    
    fd = fileno(_GET_HANDLE(context));
    do {
        result = fd_datasync(fd);
    } while (result < 0 && errno == EINTR);

    if (result < 0) {
        _cc_logger_error(_T("Error flushing datastream: %s"), strerror(errno));
        return false;
    }
    return true;
}
_CC_API_PRIVATE(int64_t) io_size(_cc_file_t *context) {
    _cc_fseek_off_t pos = 0, size = 1;
    _cc_assert(context && _GET_HANDLE(context));
    /*
    if (!context || _GET_HANDLE(context) == nullptr) {
        _cc_logger_error(_T("invalid context/file not opened"));
        return -1;
    }
    */
    if (_cc_fseek_off(_GET_HANDLE(context), 0, _CC_FILE_SEEK_CUR_) == 0) {
        pos = _cc_ftell_off(_GET_HANDLE(context));
        if (pos < 0) {
            return -1;
        }
    }

    if (_cc_fseek_off(_GET_HANDLE(context), 0, _CC_FILE_SEEK_END_) == 0) {
        size = _cc_ftell_off(_GET_HANDLE(context));
    }

    _cc_fseek_off(_GET_HANDLE(context), pos, _CC_FILE_SEEK_SET_);

    return (int64_t)size;
}

_CC_API_PRIVATE(int64_t) io_seek(_cc_file_t *context, int64_t offset, int whence) {
     const bool_t is_noop = (whence == _CC_FILE_SEEK_CUR_) && (offset == 0);
    _cc_assert(context && _GET_HANDLE(context));
    /*
    if (!context || _GET_HANDLE(context) == nullptr) {
        _cc_logger_error(_T("invalid context/file not opened"));
        return false;
    }
    */
    if (is_noop || _cc_fseek_off(_GET_HANDLE(context), (_cc_fseek_off_t)offset, whence) == 0) {
        const int64_t pos = _cc_ftell_off(_GET_HANDLE(context));
        if (pos < 0) {
            _cc_logger_error(_T("Couldn't get stream offset: %s"), strerror(errno));
            return -1;
        }
        return pos;
    }
    _cc_logger_error(_T("Error seeking in datastream: %s"), strerror(errno));
    return -1;
}

_CC_API_PRIVATE(size_t) io_read(_cc_file_t *context, pvoid_t ptr, size_t size, size_t maxnum) {
    _cc_assert(context && _GET_HANDLE(context));

    const size_t bytes = fread(ptr, size, maxnum, _GET_HANDLE(context));
    if (bytes == 0 && ferror(_GET_HANDLE(context))) {
        if (errno == EAGAIN) {
            clearerr(_GET_HANDLE(context));
        } else {
            _cc_logger_error(_T("Error reading from datastream: %s"), strerror(errno));
        }
    }
    return bytes;
}

_CC_API_PRIVATE(size_t) io_write(_cc_file_t *context, const pvoid_t ptr, size_t size, size_t num) {
    const size_t bytes = fwrite(ptr, size, num, _GET_HANDLE(context));
    if (bytes == 0 && ferror(_GET_HANDLE(context))) {
        if (errno == EAGAIN) {
            clearerr(_GET_HANDLE(context));
        } else {
            _cc_logger_error(_T("Error writing from datastream: %s"), strerror(errno));
        }
    }
    return bytes;
}

_CC_API_PRIVATE(bool_t) io_close(_cc_file_t *context) {
    bool_t status = true;

    _cc_assert(context && _GET_HANDLE(context));
    if (!context || _GET_HANDLE(context) == nullptr) {
        _cc_logger_error(_T("invalid context/file not opened"));
        return false;
    }

    /* WARNING:  Check the return value here! */
    if (fclose(_GET_HANDLE(context)) != 0) {
        status = false;
        _cc_logger_error(_T("Error closing datastream: %s"), strerror(errno));
    }

    _cc_free(context);

    return status;
}

#ifndef __CC_MACOSX__
/**/
_CC_API_PUBLIC(bool_t) _cc_sys_open_file(_cc_file_t *f, const tchar_t *filename, const tchar_t *mode) {
    f->fp = _tfopen(filename, mode);
    return (f->fp != nullptr);
}
#endif /*!(__CC_MACOSX__)*/

#endif /*!__CC_WINDOWS__*/

/**/
_CC_API_PUBLIC(_cc_file_t*) _cc_open_file(const tchar_t *filename, const tchar_t *mode) {
    _cc_file_t *f;

    f = (_cc_file_t *)_cc_malloc(sizeof(_cc_file_t));
    if (!_cc_sys_open_file(f, filename, mode)) {
        _cc_logger_error(_T("Couldn't open %s"), filename);
        _cc_free(f);
        return nullptr;
    }

#ifndef __CC_WINDOWS__
    f->read = io_read;
    f->write = io_write;
    f->size = io_size;
    f->seek = io_seek;
    f->flush = io_flush;
    f->close = io_close;
#endif

    return f;
}
