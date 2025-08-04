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
#include <libcc/atomic.h>
#include <libcc/logger.h>
#include <libcc/string.h>
#include <libcc/socket/socket.h>
#include <time.h>

#ifdef __CC_WINDOWS__
#include <libcc/core/windows.h>
#endif

#define _CC_LOG_BUFFER_SIZE_ _CC_2K_BUFFER_SIZE_

#ifdef __CC_ANDROID__
#include <libcc/core/android.h>

_CC_API_PRIVATE(void) _output_android(uint8_t level, const char_t *msg) {
    switch(level) {
        case _CC_LOG_LEVEL_EMERG_:
            __android_log_print(ANDROID_LOG_FATAL, _CC_ANDROID_TAG_, "%s", msg);
            break;
        case _CC_LOG_LEVEL_ALERT_:
            __android_log_print(ANDROID_LOG_ERROR, _CC_ANDROID_TAG_, "%s", msg);
            break;
        case _CC_LOG_LEVEL_CRIT_:
            __android_log_print(ANDROID_LOG_ERROR, _CC_ANDROID_TAG_, "%s", msg);
            break;
        case _CC_LOG_LEVEL_ERROR_:
            __android_log_print(ANDROID_LOG_ERROR, _CC_ANDROID_TAG_, "%s", msg);
            break;
        case _CC_LOG_LEVEL_WARNING_:
            __android_log_print(ANDROID_LOG_WARN, _CC_ANDROID_TAG_, "%s", msg);
            break;
        case _CC_LOG_LEVEL_NOTICE_:
            __android_log_print(ANDROID_LOG_INFO, _CC_ANDROID_TAG_, "%s", msg);
            break;
        case _CC_LOG_LEVEL_INFO_:
            __android_log_print(ANDROID_LOG_INFO, _CC_ANDROID_TAG_, "%s", msg);
            break;
        case _CC_LOG_LEVEL_DEBUG_:
            __android_log_print(ANDROID_LOG_DEBUG, _CC_ANDROID_TAG_, "%s", msg);
            break;
    }
}
#endif

const char  SYSLOG_LEVEL_CODE[_CC_LOG_LEVEL_DEBUG_ + 1] = {
    'G', 'A', 'C', 'E', 'W', 'N', 'I', 'D'
};

_CC_API_PRIVATE(void) _outputA_log(uint8_t level, const char_t *msg, size_t length) {
#ifdef __CC_ANDROID__
    _output_android(level, msg);
#else
    tchar_t buffer[_CC_8K_BUFFER_SIZE_];
    struct tm tm_now;
    time_t now = time(nullptr);
    _cc_gmtime(&now, &tm_now);

    _sntprintf(buffer, _cc_countof(buffer), _T("<%c>%04d-%02d-%02dT%02d:%02d:%02dZ %d "),
                                SYSLOG_LEVEL_CODE[level], 
                                tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday, tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec, 
                                _cc_getpid());
#ifdef _CC_MSVC_
    OutputDebugString(buffer);
    OutputDebugStringA(msg);
    OutputDebugStringA("\n");
#endif
    fputs(buffer, stdout);
    fputs(msg, stdout);
    fputs("\n", stdout);
#endif
    _cc_syslogA(level, msg, length);
}

_CC_API_PRIVATE(void) _outputW_log(uint8_t level, const wchar_t *msg, size_t length) {
#ifdef __CC_ANDROID__
    _output_android(level, msg);
#else
    tchar_t buffer[_CC_8K_BUFFER_SIZE_];
    struct tm tm_now;
    time_t now = time(nullptr);
    _cc_gmtime(&now, &tm_now);

    _sntprintf(buffer, _cc_countof(buffer), _T("<%c>%04d-%02d-%02dT%02d:%02d:%02dZ %d "),
                                SYSLOG_LEVEL_CODE[level], 
                                tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday, tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec,
                                _cc_getpid());
#ifdef _CC_MSVC_
    OutputDebugString(buffer);
    OutputDebugStringW(msg);
    OutputDebugStringW(L"\n");
#endif
    fputs(buffer, stdout);
    fputws(msg, stdout);
    fputws(L"\n", stdout);
#endif

    _cc_syslogW(level, msg, length);
}

_CC_API_PUBLIC(void) _cc_loggerA(uint8_t level, const char_t *msg) {
    _outputA_log(level, msg, strlen(msg));
}

_CC_API_PUBLIC(void) _cc_loggerW(uint8_t level, const wchar_t *msg) {
    _outputW_log(level, msg, wcslen(msg));
}

_CC_API_PUBLIC(void) _cc_loggerA_vformat(uint8_t level, const char_t *fmt, va_list arg) {
    char_t buf[_CC_LOG_BUFFER_SIZE_];
    size_t fmt_length, empty_len;
    char_t *ptr = buf;
    char_t *tmp_ptr = nullptr;

    fmt_length = 0;

    _cc_assert(fmt != nullptr);

    empty_len = _CC_LOG_BUFFER_SIZE_;
    /* If the first attempt to append fails, resize the buffer appropriately
     * and try again */
    while (true) {
        /* fmt_length is the length of the string required, excluding the
         * trailing nullptr */
        fmt_length = _vsnprintf(ptr, empty_len, fmt, arg);

#ifdef __CC_WINDOWS__
        if (fmt_length == -1) {
            fmt_length = _vsnprintf(nullptr, 0, fmt, arg);
        }
#endif
        if (fmt_length <= 0) {
            break;
        }

        /* SUCCESS */
        if (fmt_length < empty_len) {
            _outputA_log(level, ptr, fmt_length);
            break;
        }
        empty_len = _cc_aligned_alloc_opt(fmt_length + 10, 32);
        ptr = (char_t *)_cc_realloc(tmp_ptr, sizeof(char_t) * empty_len);
        tmp_ptr = ptr;
    }

    if (tmp_ptr) {
        _cc_free(tmp_ptr);
    }
}

_CC_API_PUBLIC(void) _cc_loggerW_vformat(uint8_t level, const wchar_t *fmt, va_list arg) {
    wchar_t buf[_CC_LOG_BUFFER_SIZE_];
    size_t fmt_length, empty_len;

    wchar_t *ptr = buf;
    wchar_t *tmp_ptr = nullptr;

    fmt_length = 0;

    empty_len = _CC_LOG_BUFFER_SIZE_;
    /* If the first attempt to append fails, resize the buffer appropriately
     * and try again */
    while (true) {
        /* fmt_length is the length of the string required, excluding the
         * trailing nullptr */
        fmt_length = _vsnwprintf(ptr, empty_len, fmt, arg);

#ifdef __CC_WINDOWS__
        if (fmt_length == -1) {
            fmt_length = _vsnwprintf(nullptr, 0, fmt, arg);
        }
#endif
        if (fmt_length <= 0) {
            break;
        }

        /* SUCCESS */
        if (fmt_length < empty_len) {
            _outputW_log(level, ptr, fmt_length);
            break;
        }

        empty_len = _cc_aligned_alloc_opt(fmt_length + 10,32);
        ptr = (wchar_t *)_cc_realloc(tmp_ptr, sizeof(wchar_t) * empty_len);
        tmp_ptr = ptr;
    }

    if (tmp_ptr) {
        _cc_free(tmp_ptr);
    }
}

_CC_API_PUBLIC(void) _cc_loggerA_format(uint8_t level, const char_t *fmt, ...) {
    va_list arg;

    va_start(arg, fmt);
    _cc_loggerA_vformat(level, fmt, arg);
    va_end(arg);
}

_CC_API_PUBLIC(void) _cc_loggerW_format(uint8_t level, const wchar_t *fmt, ...) {
    va_list arg;

    va_start(arg, fmt);
    _cc_loggerW_vformat(level, fmt, arg);
    va_end(arg);
}
