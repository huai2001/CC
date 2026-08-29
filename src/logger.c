#include <stdarg.h>
#include <stdio.h>
#include <libcc/alloc.h>
#include <libcc/thread.h>
#include <libcc/mutex.h>
#include <libcc/logger.h>
#include <libcc/string.h>

#define _LOGGER_BUFFER_LIMIT_  (1L << 8)
#define _LOGGER_BUFFER_LIMIT_MASK_ (_LOGGER_BUFFER_LIMIT_ - 1)
#define _LOGGER_MESSAGE_LIMIT_  (2048)

struct logger_entryA {
    uint8_t level;
    time_t timestamp;
    int32_t length;
    char message[_LOGGER_MESSAGE_LIMIT_];
};

struct logger_entryW {
    uint8_t level;
    time_t timestamp;
    int32_t length;
    wchar_t message[_LOGGER_MESSAGE_LIMIT_];
};

static struct {
    _cc_mutex_t *lock;
    struct logger_entryA buffers[_LOGGER_BUFFER_LIMIT_];
    int w;
    int r;
} ringA;

static struct {
    _cc_mutex_t *lock;
    struct logger_entryW buffers[_LOGGER_BUFFER_LIMIT_];
    int w;
    int r;
} ringW;

static _cc_thread_t *logger_thread = NULL;
static bool_t logger_running = false;

static const char *SYSLOG_LEVEL_COLORS[_CC_LOG_LEVEL_DEBUG_ + 1] = {
    //EMERG,    ALERT,      CRIT,       ERR,        WARNING,    NOTICE,     INFO,       DEBUG
    "\x1b[94m", "\x1b[31m", "\x1b[94m", "\x1b[31m", "\x1b[36m", "\x1b[32m", "\x1b[35m", "\x1b[34m"
};

static void header(uint8_t level, time_t timestamp) {
    static const char SYSLOG_LEVEL_CODE[_CC_LOG_LEVEL_DEBUG_ + 1] = {'G', 'A', 'C', 'E', 'W', 'N', 'I', 'D'};
    struct tm tm_now;
    _cc_localtime(&timestamp, &tm_now);
    printf("%s<%c>\x1b[0m\x1b[90m%04d-%02d-%02d %02d:%02d:%02d\x1b[0m ",
                                SYSLOG_LEVEL_COLORS[level],
                                SYSLOG_LEVEL_CODE[level], 
                                tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday,
                                tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);
}

static void _alog(uint8_t level, time_t timestamp, const char_t *msg, int32_t length) {
    const char_t ch = *(msg + length - sizeof(char_t));
    header(level, timestamp);
    fputs(SYSLOG_LEVEL_COLORS[level], stdout);
    fwrite(msg, sizeof(char_t), length, stdout);
    fputs("\x1b[0m", stdout);
    if (ch != '\n' && ch != '\r') {
        fputwc('\n', stdout);
    }
}

static void _wlog(uint8_t level, time_t timestamp, const wchar_t *msg, int32_t length) {
    const wchar_t ch = *(msg + length - sizeof(wchar_t));
    header(level, timestamp);
    fputs(SYSLOG_LEVEL_COLORS[level], stdout);
    fwrite(msg, sizeof(wchar_t), length, stdout);
    fputs("\x1b[0m", stdout);
    if (ch != L'\n' && ch != L'\r') {
        fputwc(L'\n', stdout);
    }
}

/**/
_CC_API_DYLIB_PRIVATE(void) __install_logger(void) {
    memset(ringW.buffers, 0, sizeof(ringW.buffers));
    memset(ringA.buffers, 0, sizeof(ringA.buffers));
    if (!ringW.lock) {
        ringW.lock = _cc_alloc_mutex();
    }
    if (!ringA.lock) {
        ringA.lock = _cc_alloc_mutex();
    }
}

/**/
_CC_API_DYLIB_PRIVATE(void) __uninstall_logger(void) {
    if (logger_running && logger_thread) {
        logger_running = false;
        _cc_wait_thread(logger_thread, NULL);
        logger_thread = NULL;
    }

    if (ringW.lock) {
        _cc_free_mutex(ringW.lock);
        ringW.lock = NULL;
    }
    if (ringA.lock) {
        _cc_free_mutex(ringA.lock);
        ringA.lock = NULL;
    }
}

_CC_API_PUBLIC(void) _cc_loggerA(const char_t *file, int line, uint8_t level, const char_t *fmt, ...) {
    char msg[_LOGGER_MESSAGE_LIMIT_];
    va_list ap;
    struct logger_entryA *log;
    int n,m = 0;    
    char_t *fname = strrchr(file, _CC_SLASH_C_);
    if (fname) {
        m = snprintf(msg, _cc_countof(msg), "%s(%d) ", fname + 1, line);
        if (m < 0 || m >= _cc_countof(msg)) {
            m = 0;
        }
    }

    va_start(ap, fmt);
    n = vsnprintf(msg + m, _cc_countof(msg) - m, fmt, ap);
    va_end(ap);

    if (n < 0 || (n == 0 && m == 0)) {
        return;
    } else if ((n + m) >= _cc_countof(msg)) {
        n = _cc_countof(msg) - m - 1;
    }

    _cc_mutex_lock(ringA.lock);
    log = &ringA.buffers[ringA.w];
    log->level = 0xff;
    ringA.w = (ringA.w + 1) & _LOGGER_BUFFER_LIMIT_MASK_;
    if (ringA.w == ringA.r) {
        ringA.r = (ringA.r + 1) & _LOGGER_BUFFER_LIMIT_MASK_;
    }
    _cc_mutex_unlock(ringA.lock);

    log->timestamp = time(NULL);
    log->length = n + m;
    memcpy(log->message, msg, log->length);
    log->message[log->length] = 0;
    log->level = level;
}

_CC_API_PUBLIC(void) _cc_loggerW(const wchar_t *file, int line, uint8_t level, const wchar_t *fmt, ...) {
    wchar_t msg[_LOGGER_MESSAGE_LIMIT_];
    va_list ap;
    struct logger_entryW *log;
    int n,m = 0;
    wchar_t *fname = wcsrchr(file, _CC_SLASH_C_);
    if (fname) {
        m = swprintf(msg, _cc_countof(msg), L"%s(%d)", fname + 1, line);
        if (m < 0 || m >= _cc_countof(msg)) {
            m = 0;
        }
    }

    va_start(ap, fmt);
    n = vswprintf(msg + m, _cc_countof(msg) - m, fmt, ap);
    va_end(ap);

    if (n < 0 || (n == 0 && m == 0)) {
        return;
    } else if ((n + m) >= _cc_countof(msg)) {
        n = _cc_countof(msg) - m - 1;
    }

    _cc_mutex_lock(ringW.lock);
    log = &ringW.buffers[ringW.w];
    log->level = 0xff;
    ringW.w = (ringW.w + 1) & _LOGGER_BUFFER_LIMIT_MASK_;
    if (ringW.w == ringW.r) {
        ringW.r = (ringW.r + 1) & _LOGGER_BUFFER_LIMIT_MASK_;
    }
    _cc_mutex_unlock(ringW.lock);

    log->timestamp = time(NULL);
    log->length = n + m;
    memcpy(log->message, msg, log->length * sizeof(wchar_t));
    log->message[log->length] = 0;
    log->level = level;
}

_CC_API_PUBLIC(void) _cc_loggerA_dump(_cc_loggerA_func_t pfun) {
    int r;
    struct logger_entryA *log;
    if (!pfun) {
        pfun = _alog;
    }
    _cc_mutex_lock(ringA.lock);
    do {
        log = &ringA.buffers[ringA.r];
        if (ringA.r == ringA.w || log->level == 0xff) {
            break;
        }
        r = ringA.r;
        _cc_mutex_unlock(ringA.lock);
        
        pfun(log->level, log->timestamp, log->message, log->length);

        _cc_mutex_lock(ringA.lock);

        if (r == ringA.r) {
            ringA.r = (ringA.r + 1) & _LOGGER_BUFFER_LIMIT_MASK_;
        }
        log->level = 0xff;
    } while (1);
    _cc_mutex_unlock(ringA.lock);
}

_CC_API_PUBLIC(void) _cc_loggerW_dump(_cc_loggerW_func_t pfun) {
    int r;
    struct logger_entryW *log;

    if (!pfun) {
        pfun = _wlog;
    }

    _cc_mutex_lock(ringW.lock);
    do {
        log = &ringW.buffers[ringW.r];
        if (ringW.r == ringW.w || log->level == 0xff) {
            break;
        }
        r = ringW.r;
        _cc_mutex_unlock(ringW.lock);

        pfun(log->level, log->timestamp, log->message, log->length);

        _cc_mutex_lock(ringW.lock);

        if (r == ringW.r) {
            ringW.r = (ringW.r + 1) & _LOGGER_BUFFER_LIMIT_MASK_;
        }
        log->level = 0xff;
    } while (1);

    _cc_mutex_unlock(ringW.lock);
}

static int _dump(void *args) {
    while (logger_running) {
        _cc_loggerA_dump(_alog);
        _cc_loggerW_dump(_wlog);
        _cc_sleep(100);
    }
    return 1;
}

_CC_API_PUBLIC(void) _cc_logger_dump(void) {
    if (logger_thread == NULL) {
        logger_running = true;
        logger_thread = _cc_thread(_dump,_T("logger dump"), NULL);
    }
}