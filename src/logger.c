#include <libcc/alloc.h>
#include <libcc/atomic.h>
#include <libcc/logger.h>
#include <libcc/string.h>
#include <libcc/socket.h>
#include <time.h>

#ifdef __CC_WINDOWS__
#include <libcc/os/windows.h>
#endif

#define _CC_LOG_BUFFER_SIZE_ _CC_2K_BUFFER_SIZE_

#ifdef __CC_ANDROID__
#include <libcc/os/android.h>

_CC_API_PRIVATE(void) _output_android(const tchar_t *fname, int line, uint8_t level, const char_t *msg) {
    switch(level) {
        case _CC_LOG_LEVEL_EMERG_:
            __android_log_print(ANDROID_LOG_FATAL, _CC_ANDROID_TAG_, "%s(%d) %s", fname, line, msg);
            break;
        case _CC_LOG_LEVEL_ALERT_:
            __android_log_print(ANDROID_LOG_ERROR, _CC_ANDROID_TAG_, "%s(%d) %s", fname, line, msg);
            break;
        case _CC_LOG_LEVEL_CRIT_:
            __android_log_print(ANDROID_LOG_ERROR, _CC_ANDROID_TAG_, "%s(%d) %s", fname, line, msg);
            break;
        case _CC_LOG_LEVEL_ERROR_:
            __android_log_print(ANDROID_LOG_ERROR, _CC_ANDROID_TAG_, "%s(%d) %s", fname, line, msg);
            break;
        case _CC_LOG_LEVEL_WARNING_:
            __android_log_print(ANDROID_LOG_WARN, _CC_ANDROID_TAG_, "%s(%d) %s", fname, line, msg);
            break;
        case _CC_LOG_LEVEL_NOTICE_:
            __android_log_print(ANDROID_LOG_INFO, _CC_ANDROID_TAG_, "%s(%d) %s", fname, line, msg);
            break;
        case _CC_LOG_LEVEL_INFO_:
            __android_log_print(ANDROID_LOG_INFO, _CC_ANDROID_TAG_, "%s(%d) %s", fname, line, msg);
            break;
        case _CC_LOG_LEVEL_DEBUG_:
            __android_log_print(ANDROID_LOG_DEBUG, _CC_ANDROID_TAG_, "%s(%d) %s", fname, line, msg);
            break;
    }
}
#endif

const char  SYSLOG_LEVEL_CODE[_CC_LOG_LEVEL_DEBUG_ + 1] = {
    'G', 'A', 'C', 'E', 'W', 'N', 'I', 'D'
};

_CC_API_PUBLIC(void) _cc_loggerA(const tchar_t *file, int line, uint8_t level, const char_t *msg, size_t length) {
#ifndef __CC_ANDROID__
    tchar_t buffer[_CC_1K_BUFFER_SIZE_];
    struct tm tm_now;
    time_t now = time(NULL);
#endif
    const tchar_t *fname = _tcsrchr(file, _CC_SLASH_C_);
    if (fname == NULL) {
    #ifdef __CC_WINDOWS__
        fname = _tcsrchr(file, '/');
    #else
        fname = _tcsrchr(file, '\\');
    #endif
        if (fname == NULL) {
            fname = file;
        } else {
            fname++;
        }
    } else {
        fname++;
    }

#ifdef __CC_ANDROID__
    _output_android(fname, line, level, msg);
#else
    _cc_gmtime(&now, &tm_now);
    _sntprintf(buffer, _cc_countof(buffer), _T("<%c>%04d-%02d-%02dT%02d:%02d:%02dZ %d %s(%d) "),
                                SYSLOG_LEVEL_CODE[level], 
                                tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday, tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec, 
                                _cc_getpid(), fname, line);
#ifdef __CC_MSVC__
    OutputDebugString(buffer);
    OutputDebugStringA(msg);
    OutputDebugStringA("\n");
#endif
    fputs(buffer, stdout);
    fputs(msg, stdout);
    fputc('\n', stdout);
#endif

#ifdef _CC_USE_SYSLOG_
    _cc_syslogA(level, msg, length);
#endif
}
/*
"\033[30m Black     \033[0m"
"\033[31m Red       \033[0m"
"\033[32m Green     \033[0m"
"\033[33m Yellow    \033[0m"
"\033[34m Blue      \033[0m"
"\033[35m Pink      \033[0m"
"\033[36m Sky blue  \033[0m"
"\033[37m White     \033[0m"
*/
_CC_API_PUBLIC(void) _cc_loggerW(const tchar_t *file, int line, uint8_t level, const wchar_t *msg, size_t length) {
#ifndef __CC_ANDROID__
    tchar_t buffer[_CC_1K_BUFFER_SIZE_];
    struct tm tm_now;
    time_t now = time(NULL);
#endif
    const tchar_t *fname = _tcsrchr(file, _CC_SLASH_C_);
    if (fname == NULL) {
    #ifdef __CC_WINDOWS__
        fname = _tcsrchr(file, '/');
    #else
        fname = _tcsrchr(file, '\\');
    #endif
        if (fname == NULL) {
            fname = file;
        } else {
            fname++;
        }
    } else {
        fname++;
    }
#ifdef __CC_ANDROID__
    //_output_android(fname, line, level, msg);
#else
    _cc_gmtime(&now, &tm_now);
    _sntprintf(buffer, _cc_countof(buffer), _T("<%c>%04d-%02d-%02dT%02d:%02d:%02dZ %d %s(%d) "),
                                SYSLOG_LEVEL_CODE[level], 
                                tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday, tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec,
                                _cc_getpid(), fname, line);
#ifdef __CC_MSVC__
    OutputDebugString(buffer);
    OutputDebugStringW(msg);
    OutputDebugStringW(L"\n");
#endif
    fputs(buffer, stdout);
    fputws(msg, stdout);
    fputc('\n', stdout);
#endif

#ifdef _CC_USE_SYSLOG_
    _cc_syslogW(level, msg, length);
#endif
}

_CC_API_PUBLIC(void) _cc_loggerA_vformat(const tchar_t *file, int line, uint8_t level, const char_t *fmt, va_list arg) {
    char_t buf[_CC_LOG_BUFFER_SIZE_];
    int fmt_length, remaining;
    char_t *ptr = buf;
    char_t *tmp_ptr = NULL;

    fmt_length = 0;

    _cc_assert(fmt != NULL);

    remaining = _CC_LOG_BUFFER_SIZE_;
    /* If the first attempt to append fails, resize the buffer appropriately
     * and try again */
    do {
        /* fmt_length is the length of the string required, excluding the
         * trailing NULL */
        va_list arg_copy;
        va_copy(arg_copy, arg);
        fmt_length = (int)_vsnprintf(ptr, remaining, fmt, arg_copy);
        va_end(arg_copy);

#ifdef __CC_WINDOWS__
        if (fmt_length == -1) {
            va_copy(arg_copy, arg);
            fmt_length = (int)_vsnprintf(NULL, 0, fmt, arg_copy);
            va_end(arg_copy);
        }
#endif
        if (fmt_length <= 0) {
            break;
        }

        /* SUCCESS */
        if (fmt_length < remaining) {
            _cc_loggerA(file, line, level, ptr, fmt_length);
            break;
        }
        remaining = fmt_length;
        ptr = (char_t *)_cc_realloc(tmp_ptr, sizeof(char_t) * remaining);
        tmp_ptr = ptr;
    } while (true);

    if (tmp_ptr) {
        _cc_free(tmp_ptr);
    }
}

_CC_API_PUBLIC(void) _cc_loggerW_vformat(const tchar_t *file, int line, uint8_t level, const wchar_t *fmt, va_list arg) {
    wchar_t buf[_CC_LOG_BUFFER_SIZE_];
    int fmt_length, remaining;

    wchar_t *ptr = buf;
    wchar_t *tmp_ptr = NULL;

    fmt_length = 0;

    remaining = _CC_LOG_BUFFER_SIZE_;
    /* If the first attempt to append fails, resize the buffer appropriately
     * and try again */
    do {
        /* fmt_length is the length of the string required, excluding the
         * trailing NULL */
        va_list arg_copy;
        va_copy(arg_copy, arg);
        fmt_length = (int)_vsnwprintf(ptr, remaining, fmt, arg_copy);
        va_end(arg_copy);

#ifdef __CC_WINDOWS__
        if (fmt_length == -1) {
            va_copy(arg_copy, arg);
            fmt_length = (int)_vsnwprintf(NULL, 0, fmt, arg_copy);
            va_end(arg_copy);
        }
#endif
        if (fmt_length <= 0) {
            break;
        }

        /* SUCCESS */
        if (fmt_length < remaining) {
            _cc_loggerW(file, line, level, ptr, fmt_length);
            break;
        }

        remaining = fmt_length;
        ptr = (wchar_t *)_cc_realloc(tmp_ptr, sizeof(wchar_t) * remaining);
        tmp_ptr = ptr;
    } while (true);

    if (tmp_ptr) {
        _cc_free(tmp_ptr);
    }
}

_CC_API_PUBLIC(void) _cc_loggerA_format(const tchar_t *file, int line, uint8_t level, const char_t *fmt, ...) {
    va_list arg;

    va_start(arg, fmt);
    _cc_loggerA_vformat(file, line, level, fmt, arg);
    va_end(arg);
}

_CC_API_PUBLIC(void) _cc_loggerW_format(const tchar_t *file, int line, uint8_t level, const wchar_t *fmt, ...) {
    va_list arg;

    va_start(arg, fmt);
    _cc_loggerW_vformat(file, line, level, fmt, arg);
    va_end(arg);
}
