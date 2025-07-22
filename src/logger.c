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

#ifdef __CC_ANDROID__
#include <libcc/core/android.h>
#endif

#ifdef _CC_MSVC_
#include <libcc/core/windows.h>
#endif

#define _CC_LOG_BUFFER_SIZE_ _CC_2K_BUFFER_SIZE_

static struct {
    bool_t enabled;
    _cc_socket_t fd;
    _cc_sockaddr_t sockaddr;
    _cc_atomic_lock_t lock;

    uint32_t pid;

    tchar_t hostname[64];
    tchar_t app_name[64];

} syslog = {0};

#ifdef __CC_ANDROID__
_CC_API_PRIVATE(void) _output_android(byte_t level, const char_t *msg, size_t length) {
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

/**/
_CC_API_PUBLIC(void) _cc_logger_lock(void) {
    _cc_spin_lock(&syslog.lock);
}

/**/
_CC_API_PUBLIC(void) _cc_logger_unlock() {
    _cc_unlock(&syslog.lock);
}

_CC_API_PUBLIC(void) _cc_loggerA(byte_t level, const char_t *msg) {
    _cc_loggerA_syslog(_CC_LOGGER_PRI(_CC_LOG_FACILITY_USER_,level), msg, strlen(msg));
}

_CC_API_PUBLIC(void) _cc_loggerW(byte_t level, const wchar_t *msg) {
    _cc_loggerW_syslog(_CC_LOGGER_PRI(_CC_LOG_FACILITY_USER_,level), msg, wcslen(msg));
}

_CC_API_PUBLIC(void) _cc_loggerA_vformat(byte_t level, const char_t *fmt, va_list arg) {
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
        if (_cc_unlikely(fmt_length <= 0)) {
            break;
        }

        /* SUCCESS */
        if (fmt_length < empty_len) {
            _cc_loggerA_syslog(_CC_LOGGER_PRI(_CC_LOG_FACILITY_USER_,level), ptr, fmt_length);
            break;
        }
        empty_len = _cc_aligned_alloc_opt(fmt_length + 10,32);
        ptr = (char_t *)_cc_realloc(tmp_ptr, sizeof(char_t) * empty_len);
        tmp_ptr = ptr;
    }

    if (tmp_ptr) {
        _cc_free(tmp_ptr);
    }
}

_CC_API_PUBLIC(void) _cc_loggerW_vformat(byte_t level, const wchar_t *fmt, va_list arg) {
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
        if (_cc_unlikely(fmt_length <= 0)) {
            break;
        }

        /* SUCCESS */
        if (fmt_length < empty_len) {
            _cc_loggerW_syslog(_CC_LOGGER_PRI(_CC_LOG_FACILITY_USER_,level), ptr, fmt_length);
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

_CC_API_PUBLIC(void) _cc_loggerA_format(byte_t level, const char_t *fmt, ...) {
    va_list arg;

    va_start(arg, fmt);
    _cc_loggerA_vformat(level, fmt, arg);
    va_end(arg);
}

_CC_API_PUBLIC(void) _cc_loggerW_format(byte_t level, const wchar_t *fmt, ...) {
    va_list arg;

    va_start(arg, fmt);
    _cc_loggerW_vformat(level, fmt, arg);
    va_end(arg);
}

_CC_API_PUBLIC(void) _cc_logger_set_hostname(const tchar_t *hostname) {
    _tcsncpy(syslog.hostname, hostname, _cc_countof(syslog.hostname));
    syslog.hostname[_cc_countof(syslog.hostname) - 1] = 0;
}

_CC_API_PUBLIC(void) _cc_logger_set_app_name(const tchar_t *app_name) {
    _tcsncpy(syslog.app_name, app_name, _cc_countof(syslog.app_name));
    syslog.app_name[_cc_countof(syslog.app_name) - 1] = 0;
}

_CC_API_PUBLIC(void) _cc_loggerW_syslog(byte_t pri, const wchar_t* msg, size_t length) {
    //wchar_t str_date[128];
    _cc_buf_t buffer;
    struct tm tm_now;
    time_t now = time(nullptr);

    _cc_gmtime(&now, &tm_now);

    //_cc_buf_cleanup(&syslog.buffer);
    _cc_buf_alloc(&buffer, _CC_16K_BUFFER_SIZE_);

#ifdef _RFC_3164_
    // RFC 3164
    result = wcsftime(str_date, _cc_countof(str_date), L"%b %d %H:%M:%S", tm_now);
    _cc_bufW_appendf(&buffer, L"<%d>%s %s[%d] ",
                    pri, str_date, syslog.hostname, syslog.pid);
#else
    // RFC 5424
    _cc_bufW_appendf(&buffer, L"<%d>%d %04d-%02d-%02dT%02d:%02d:%02dZ %s %s %d %d ",
                    pri, _CC_SYSLOG_VERSIOV_, 
                    tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday,
                    tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec, 
                    syslog.hostname, syslog.app_name, syslog.pid, syslog.fd);
#endif

    if (msg) {
        if (length <= 0) {
            length = wcslen(msg);
        }
        _cc_buf_append(&buffer, msg, length * sizeof(wchar_t));
    }

    if ((pri & 0x7) == _CC_LOG_LEVEL_ERROR_) {
        #ifdef _CC_MSVC_
        _cc_getW_resolve_symbol(&buffer);
        #elif defined(_CC_GCC_) && !defined(__CC_MINGW__)
        _cc_get_resolve_symbol(&buffer);
        #endif
    }

    buffer.bytes[buffer.length] = 0;
#ifdef __CC_WINDOWS__
    OutputDebugStringW((const wchar_t *)buffer.bytes);
    OutputDebugStringW(L"\n");
#else
    fputws((const wchar_t *)buffer.bytes, stdout);
    fputs("\n", stdout);
#endif

    _cc_buf_utf16_to_utf8(&buffer, 0);

    _cc_syslog(buffer.bytes, buffer.length);
    _cc_buf_free(&buffer);
}

/**/
_CC_API_PUBLIC(void) _cc_loggerA_syslog(byte_t pri, const char_t* msg, size_t length) {
    //char_t str_date[128];
    _cc_buf_t buffer;
    struct tm tm_now;
    time_t now = time(nullptr);

    _cc_gmtime(&now, &tm_now);

    //_cc_buf_cleanup(&syslog.buffer);
    _cc_buf_alloc(&buffer, _CC_16K_BUFFER_SIZE_);

#ifdef _RFC_3164_
    // RFC 3164
    result = strftime(str_date, _cc_countof(str_date), "%b %d %H:%M:%S", tm_now);
    _cc_bufA_appendf(&buffer, "<%d>%s %s[%d] ",
                    pri, str_date, syslog.hostname, syslog.pid);
#else
    // RFC 5424
    _cc_bufA_appendf(&buffer, "<%d>%d %04d-%02d-%02dT%02d:%02d:%02dZ %s %s %d %d ",
                    pri, _CC_SYSLOG_VERSIOV_, 
                    tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday,
                    tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec, 
                    syslog.hostname, syslog.app_name, syslog.pid, syslog.fd);
#endif

    if (msg) {
        size_t i = 0;
        if (length <= 0) {
            length = strlen(msg);
        }
        _cc_buf_append(&buffer, msg, length * sizeof(char_t));
    }

    if ((pri & 0x7) == _CC_LOG_LEVEL_ERROR_) {
        #ifdef _CC_MSVC_
        _cc_getA_resolve_symbol(&buffer);
        #elif defined(_CC_GCC_) && !defined(__CC_MINGW__)
        _cc_get_resolve_symbol(&buffer);
        #endif
    }

    buffer.bytes[buffer.length] = 0;
#ifdef __CC_WINDOWS__
    OutputDebugStringA((const char_t *)buffer.bytes);
    OutputDebugStringW(L"\n");
#else
    fputs((const char_t *)buffer.bytes, stdout);
    fputs("\n", stdout);
#endif
    
    _cc_syslog(buffer.bytes, buffer.length);
    _cc_buf_free(&buffer);
}

/**/
_CC_API_PUBLIC(void) _cc_syslog(const byte_t* msg, size_t length) {
    if (syslog.enabled == false || syslog.fd == _CC_INVALID_SOCKET_) {
        return;
    }
    _cc_logger_lock();
    _cc_sendto(syslog.fd, msg, (int32_t)length, &syslog.sockaddr, sizeof(struct sockaddr_in));
    _cc_logger_unlock();
}

/**/
_CC_API_PUBLIC(void) _cc_logger_open_syslog(tchar_t *app_name, const tchar_t *ip, const uint16_t port) {
    if (syslog.enabled) {
        return;
    }

    syslog.fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (syslog.fd == _CC_INVALID_SOCKET_) {
        _cc_logger_error(_T("Failed to create UDP socket: %d"), _cc_last_errno());
        return;
    }

    _cc_lock_init(&syslog.lock);
    _cc_inet_ipv4_addr((struct sockaddr_in*) & syslog.sockaddr, ip, port);

    syslog.pid = _cc_getpid();
    syslog.enabled = true;
    
    _tcsncpy(syslog.app_name, app_name, _cc_countof(syslog.app_name));
    syslog.app_name[_cc_countof(syslog.app_name) - 1] = 0;

    if (gethostname(syslog.hostname, _cc_countof(syslog.hostname))) {
        _tcsncpy(syslog.hostname, _T("-"), _cc_countof(syslog.hostname));
        syslog.hostname[_cc_countof(syslog.hostname) - 1] = 0;
    }
}

/**/
_CC_API_PUBLIC(void) _cc_logger_close_syslog() {
    if (syslog.enabled == false) {
        return;
    }

    _cc_logger_lock();

    if (syslog.fd) {
        _cc_close_socket(syslog.fd);
    }

    syslog.enabled = false;
    _cc_logger_unlock();
}