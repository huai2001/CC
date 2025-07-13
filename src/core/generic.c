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
#include <libcc/dirent.h>
#include <libcc/math.h>

#ifndef __CC_WINDOWS__

#include <sys/errno.h>

_CC_API_PUBLIC(int32_t) _cc_a2w(const char_t *s1, int32_t s1_len, wchar_t *s2, int32_t size) {
    return _cc_utf8_to_utf16((const uint8_t *)s1, (const uint8_t *)(s1 + s1_len), (uint16_t *)s2,
                             (uint16_t *)(s2 + size), false);
}

_CC_API_PUBLIC(int32_t) _cc_w2a(const wchar_t *s1, int32_t s1_len, char_t *s2, int32_t size) {
    return _cc_utf16_to_utf8((const uint16_t *)s1, (const uint16_t *)(s1 + s1_len), (uint8_t *)s2,
                             (uint8_t *)(s2 + size), false);
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

_CC_API_PUBLIC(size_t) _cc_get_cwd(tchar_t *cwd, size_t length) {
    if (getcwd(cwd, length) != nullptr) {
        return _tcslen(cwd);
    }
    return 0;
}

#endif /* !__CC_WINDOWS__ */

/**/
_CC_API_PUBLIC(bool_t) _cc_mkdir(const tchar_t *path) {
    _CC_API_PUBLIC(return) _cc_create_directory(path, false);
}

/**/
_CC_API_PUBLIC(bool_t) _cc_create_directory(const tchar_t *path, bool_t is_dir) {
    int32_t i = 0;
    const tchar_t *p = nullptr;
    tchar_t wd[_CC_2K_BUFFER_SIZE_ + 1];

    p = path;
    
    /* Skip the first / */
    if (_CC_IS_SLASH(*p)) {
        wd[i++] = *p++;
        if (!*p) {
            return true;
        }
    }

    /**/
    while (*p && i < _CC_2K_BUFFER_SIZE_) {
        if (_CC_IS_SLASH(*p)) {
            wd[i] = 0;
            if (_taccess(wd, _CC_ACCESS_F_) == -1) {
                _tmkdir(wd);
            }
        }
        wd[i++] = *p++;
    }

    if (is_dir) {
        wd[i] = 0;
        if (_taccess(wd, 0) != 0) {
            _tmkdir(wd);
        }
    }
    return true;
}

_CC_API_PUBLIC(size_t) _cc_nextpow2(size_t n) {
    if (n <= 1) {
        return 1;
    }
    --n;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;

    return ++n;
}

_CC_API_PUBLIC(size_t) _cc_fpath(tchar_t *buf, size_t size, const tchar_t *fmt, ...) {
    va_list arg;
    tchar_t stack_buf[_CC_4K_BUFFER_SIZE_];
    tchar_t *ptr = stack_buf;
    tchar_t *tmp_ptr = nullptr;
    size_t fmt_length = 0, empty_length = _CC_4K_BUFFER_SIZE_;

    _cc_assert(fmt != nullptr && buf != nullptr && size > 0);

    va_start(arg, fmt);
    /* If the first attempt to append fails, resize the buffer appropriately
     * and try again */
    while (true) {
        /* fmt_length is the length of the string required, excluding the
         * trailing nullptr */
        fmt_length = _vsntprintf(ptr, empty_length, fmt, arg);

#ifdef __CC_WINDOWS__
        if (fmt_length == -1) {
            fmt_length = _vsntprintf(nullptr, 0, fmt, arg);
        }
#endif
        if (_cc_unlikely(fmt_length <= 0)) {
            goto PATH_FAIL;
        }

        /* SUCCESS */
        if (fmt_length < empty_length) {
            break;
        }
        
        empty_length = _cc_aligned_alloc_opt(fmt_length + 10, 32);
        ptr = (tchar_t *)_cc_realloc(tmp_ptr, sizeof(tchar_t) * empty_length);
        tmp_ptr = ptr;
    }
    va_end(arg);
    
#ifdef __CC_WINDOWS__
    if (_tfullpath(buf, ptr, size) == nullptr) {
#else
    if (realpath(ptr, buf) == nullptr) {
#endif
        fmt_length = _min(size, fmt_length);
        memcpy(buf, ptr, fmt_length);
        buf[fmt_length - 1] = 0;
    } else {
        fmt_length = _tcslen(buf);
    }

PATH_FAIL:
    if (tmp_ptr) {
        _cc_free(tmp_ptr);
    }
    return fmt_length;
}

_CC_API_PUBLIC(void) _cc_replace_slashes(tchar_t* path) {
    tchar_t *p = path;
    while (*p != 0) {
        if (_CC_IS_SLASH(*p)) {
            *p = _CC_SLASH_C_;
        }
    }
}