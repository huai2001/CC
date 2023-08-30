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
#include <cc/logger.h>
#include <cc/string.h>
#include <cc/atomic.h>
#include <time.h>

#ifdef __CC_ANDROID__
#include <cc/core/android.h>
#endif

#ifdef _CC_MSVC_
#include <cc/core/windows.h>
#endif

#define _CC_LOGGER_BUFFER_SIZE_ 1024

static void _print_loggerW_callback(uint16_t flags, const wchar_t *logstr, size_t len, pvoid_t userdata);
static void _print_loggerA_callback(uint16_t flags, const char_t *logstr, size_t len, pvoid_t userdata);

static struct {
    _cc_loggerA_callback_t callbackA;
    _cc_loggerW_callback_t callbackW;
    pvoid_t userdata;
    _cc_spinlock_t lock;
} _logger = {_print_loggerA_callback, _print_loggerW_callback, NULL, 0};

#ifdef __CC_ANDROID__
static void _print_loggerA_callback(uint16_t flags, const char_t *logstr, size_t len, pvoid_t userdata) {
    // ANDROID_LOG_FATAL
    if (flags & _CC_LOGGER_FLAGS_NORMAL_) {
        __android_log_print(ANDROID_LOG_INFO, _CC_ANDROID_TAG_, "%s", logstr);
    } else if (flags & _CC_LOGGER_FLAGS_DEBUG_) {
        __android_log_print(ANDROID_LOG_DEBUG, _CC_ANDROID_TAG_, "%s", logstr);
    } else if (flags & _CC_LOGGER_FLAGS_ERROR_) {
        __android_log_print(ANDROID_LOG_ERROR, _CC_ANDROID_TAG_, "%s", logstr);
    } else if (flags & _CC_LOGGER_FLAGS_WARNING_) {
        __android_log_print(ANDROID_LOG_WARN, _CC_ANDROID_TAG_, "%s", logstr);
    } else if (flags & _CC_LOGGER_FLAGS_INFO_) {
        __android_log_print(ANDROID_LOG_INFO, _CC_ANDROID_TAG_, "%s", logstr);
    }
}
static void _print_loggerW_callback(uint16_t flags, const wchar_t *logstr, size_t len, pvoid_t userdata) {
}
#else
static void _logger_print_header(uint16_t flags) {
    time_t now_time = time(NULL);
    struct tm *t = localtime(&now_time);

    fprintf(stderr, "[%4d-%02d-%02d %02d:%02d:%02d]", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour,
            t->tm_min, t->tm_sec);

    if (flags & _CC_LOGGER_FLAGS_NORMAL_) {
        fputs("[NORMAL]", stderr);
    } else if (flags & _CC_LOGGER_FLAGS_DEBUG_) {
        fputs("[DEBUG]", stderr);
    } else if (flags & _CC_LOGGER_FLAGS_ERROR_) {
        fputs("[ERROR]", stderr);
    } else if (flags & _CC_LOGGER_FLAGS_WARNING_) {
        fputs("[WARNING]", stderr);
    } else if (flags & _CC_LOGGER_FLAGS_INFO_) {
        fputs("[INFO]", stderr);
    }
}

static void _print_loggerA_callback(uint16_t flags, const char_t *logstr, size_t len, pvoid_t userdata) {
    _logger_print_header(flags);

    fputs(logstr, stderr);

    if (*(logstr + len - 1) != _CC_LF_) {
        fputc(_CC_LF_, stderr);
    }

    if (flags & _CC_LOGGER_FLAGS_ERROR_) {
        _cc_print_stack_trace(stderr, 4);
    }

#ifdef _CC_MSVC_
    OutputDebugStringA(logstr);
#endif
}

static void _print_loggerW_callback(uint16_t flags, const wchar_t *logstr, size_t len, pvoid_t userdata) {
    _logger_print_header(flags);

    fputws(logstr, stderr);

    if (*(logstr + len - 1) != _CC_LF_) {
        fputc(_CC_LF_, stderr);
    }

    if (flags & _CC_LOGGER_FLAGS_ERROR_) {
        _cc_print_stack_trace(stderr, 4);
    }

#ifdef _CC_MSVC_
    OutputDebugStringW(logstr);
#endif
}
#endif

/**/
void _cc_logger_lock(void) {
    _cc_spin_lock(&_logger.lock);
}

/**/
void _cc_logger_unlock() {
    _cc_spin_unlock(&_logger.lock);
}

/**/
void _cc_loggerA_set_output_callback(_cc_loggerA_callback_t callback, pvoid_t userdata) {
    _logger.callbackA = callback;
    _logger.userdata = userdata;
}

/**/
void _cc_loggerW_set_output_callback(_cc_loggerW_callback_t callback, pvoid_t userdata) {
    _logger.callbackW = callback;
    _logger.userdata = userdata;
}

/**/
void _cc_logger_set_output_callback(_cc_loggerA_callback_t callbackA, _cc_loggerW_callback_t callbackW,
                                    pvoid_t userdata) {
    _logger.callbackA = callbackA;
    _logger.callbackW = callbackW;
    _logger.userdata = userdata;
}

void _cc_loggerA(uint16_t flags, const char_t *str) {
    if (_cc_likely(_logger.callbackA)) {
        _cc_logger_lock();
#ifdef __CC_WINDOWS__
        flags |= _CC_LOGGER_FLAGS_ASIC_;
#else
        flags |= _CC_LOGGER_FLAGS_UTF8_;
#endif
        _logger.callbackA(flags, str, strlen(str), _logger.userdata);
        _cc_logger_unlock();
    }
}

void _cc_loggerW(uint16_t flags, const wchar_t *str) {
    if (_cc_likely(_logger.callbackW)) {
        _cc_logger_lock();
        _logger.callbackW(flags | _CC_LOGGER_FLAGS_UTF16_, str, wcslen(str), _logger.userdata);
        _cc_logger_unlock();
    }
}

void _cc_loggerA_vformat(uint16_t flags, const char_t *fmt, va_list arg) {
    static char_t buf[_CC_LOGGER_BUFFER_SIZE_];
    size_t fmt_length, empty_len;
    char_t *ptr = buf;
    char_t *tmp_ptr = NULL;
    if (_cc_unlikely(_logger.callbackA == NULL)) {
        return;
    }

    fmt_length = 0;
    _cc_logger_lock();

#ifdef __CC_WINDOWS__
    flags |= _CC_LOGGER_FLAGS_ASIC_;
#else
    flags |= _CC_LOGGER_FLAGS_UTF8_;
#endif

    _cc_assert(fmt != NULL);

    empty_len = _CC_LOGGER_BUFFER_SIZE_ - 1;
    /* If the first attempt to append fails, resize the buffer appropriately
     * and try again */
    while (true) {
        /* fmt_length is the length of the string required, excluding the
         * trailing NULL */
        fmt_length = _vsnprintf(ptr, empty_len, fmt, arg);

#ifdef __CC_WINDOWS__
        if (fmt_length == -1) {
            fmt_length = _vsnprintf(nullptr, 0, fmt, arg);
        }
#endif
        if (_cc_unlikely(fmt_length <= 0)) { 
            break;
        }

        /* SUCCESS */
        if (fmt_length < empty_len) {
            _logger.callbackA(flags, ptr, fmt_length, _logger.userdata);
            break;
        }

        ptr = (char_t *)_cc_realloc(tmp_ptr, sizeof(char_t) * (fmt_length + 10));
        tmp_ptr = ptr;
        empty_len = fmt_length + 10;
    }
    _cc_logger_unlock();

    if (tmp_ptr) {
        _cc_free(tmp_ptr);
    }
}

void _cc_loggerW_vformat(uint16_t flags, const wchar_t *fmt, va_list arg) {
    static wchar_t buf[_CC_LOGGER_BUFFER_SIZE_];
    size_t fmt_length, empty_len;

    wchar_t *ptr = buf;
    wchar_t *tmp_ptr = NULL;

    if (_cc_likely(_logger.callbackW == NULL)) {
        return;
    }

    fmt_length = 0;
    _cc_logger_lock();

    empty_len = _CC_LOGGER_BUFFER_SIZE_;
    flags |= _CC_LOGGER_FLAGS_UTF16_;
    /* If the first attempt to append fails, resize the buffer appropriately
     * and try again */
    while (true) {
        /* fmt_length is the length of the string required, excluding the
         * trailing NULL */
        fmt_length = _vsnwprintf(ptr, empty_len, fmt, arg);

#ifdef __CC_WINDOWS__
        if (fmt_length == -1) {
            fmt_length = _vsnwprintf(nullptr, 0, fmt, arg);
        }
#endif
        if (_cc_unlikely(fmt_length <= 0)) { 
            break;
        }

        /* SUCCESS */
        if (fmt_length < empty_len) {
            _logger.callbackW(flags, ptr, fmt_length, _logger.userdata);
            break;
        }

        ptr = (wchar_t *)_cc_realloc(tmp_ptr, sizeof(wchar_t) * (fmt_length + 10));
        tmp_ptr = ptr;
        empty_len = fmt_length + 10;
    }
    _cc_logger_unlock();

    if (tmp_ptr) {
        _cc_free(tmp_ptr);
    }
}

void _cc_loggerA_format(uint16_t flags, const char_t *fmt, ...) {
    va_list arg;

    va_start(arg, fmt);
    _cc_loggerA_vformat(flags, fmt, arg);
    va_end(arg);
}

void _cc_loggerW_format(uint16_t flags, const wchar_t *fmt, ...) {
    va_list arg;

    va_start(arg, fmt);
    _cc_loggerW_vformat(flags, fmt, arg);
    va_end(arg);
}
