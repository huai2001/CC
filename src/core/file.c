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
#include <cc/core.h>
#include <cc/logger.h>

/**/
bool_t _cc_sys_open_file(_cc_file_t *f, const tchar_t *filename, const tchar_t *mode);

#ifndef __CC_WINDOWS__

#define _GET_HANDLE(x) (FILE *)(x->fp)

static int64_t _cc_stdio_file_size(_cc_file_t *context) {
    _cc_fseek_off_t pos = 0, size = 1;
    _cc_assert(context && _GET_HANDLE(context));
    /*
    if (!context || _GET_HANDLE(context) == NULL) {
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

static bool_t _cc_stdio_file_seek(_cc_file_t *context, int64_t offset, int whence) {
    _cc_assert(context && _GET_HANDLE(context));
    /*
    if (!context || _GET_HANDLE(context) == NULL) {
        _cc_logger_error(_T("invalid context/file not opened"));
        return false;
    }
    */
    if (_cc_fseek_off(_GET_HANDLE(context), (_cc_fseek_off_t)offset, whence) == 0) {
        return true;
    }
    return false;
}

static size_t _cc_stdio_file_read(_cc_file_t *context, pvoid_t ptr, size_t size, size_t maxnum) {
    _cc_assert(context && _GET_HANDLE(context));
    /*
    if (!context || _GET_HANDLE(context) == NULL) {
        return -1;
    }
    */
    return fread(ptr, size, maxnum, _GET_HANDLE(context));
}

static size_t _cc_stdio_file_write(_cc_file_t *context, const pvoid_t ptr, size_t size, size_t num) {
    _cc_assert(context && _GET_HANDLE(context));
    /*
    if (!context || _GET_HANDLE(context) == NULL) {
        return -1;
    }
    */
    return fwrite(ptr, size, num, _GET_HANDLE(context));
}

static bool_t _cc_stdio_file_close(_cc_file_t *context) {
    bool_t status = true;

    _cc_assert(context && _GET_HANDLE(context));
    if (!context || _GET_HANDLE(context) == NULL) {
        _cc_logger_error(_T("invalid context/file not opened"));
        return false;
    }

    /* WARNING:  Check the return value here! */
    if (fclose(_GET_HANDLE(context)) != 0) {
        status = false;
    }

    _cc_free(context);

    return status;
}

#ifndef __CC_MACOSX__
/**/
bool_t _cc_sys_open_file(_cc_file_t *f, const tchar_t *filename, const tchar_t *mode) {
    f->fp = _tfopen(filename, mode);
    return (f->fp != NULL);
}
#endif /*!(__CC_MACOSX__ and __CC_ANDROID__)*/

#endif /*!__CC_WINDOWS__*/

/**/
_cc_file_t *_cc_open_file(const tchar_t *filename, const tchar_t *mode) {
    _cc_file_t *f;

    f = (_cc_file_t *)_cc_malloc(sizeof(_cc_file_t));
    if (!_cc_sys_open_file(f, filename, mode)) {
        _cc_logger_error(_T("Couldn't open %s"), filename);
        _cc_free(f);
        return NULL;
    }

#ifndef __CC_WINDOWS__
    f->read = _cc_stdio_file_read;
    f->write = _cc_stdio_file_write;
    f->size = _cc_stdio_file_size;
    f->seek = _cc_stdio_file_seek;
    f->close = _cc_stdio_file_close;
#endif

    return f;
}
