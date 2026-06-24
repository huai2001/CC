#include <stdarg.h>
#include <stdio.h>
#include <libcc/alloc.h>
#include <libcc/thread.h>
#include <libcc/mutex.h>
#include <libcc/logger.h>
#include <libcc/string.h>

#ifdef __CC_WINDOWS__
#include <libcc/os/windows.h>
#endif

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

/**/
void __install_logger(void) {
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
void __uninstall_logger(void) {
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
    char tmp[_LOGGER_MESSAGE_LIMIT_];
    va_list ap;
    struct logger_entryA *log;
    int n,m = 0;    
    char_t *fname = strrchr(file, _CC_SLASH_C_);
    if (fname) {
        m = snprintf(tmp, _cc_countof(tmp), "%s(%d) ", fname + 1, line);
    }

    va_start(ap, fmt);
    n = vsnprintf(tmp + m, _cc_countof(tmp) - m, fmt, ap);
    va_end(ap);

    if (n < 0) {
        return;
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
    memcpy(log->message, tmp, log->length);
    log->message[log->length] = 0;
    log->level = level;
}

_CC_API_PUBLIC(void) _cc_loggerW(const wchar_t *file, int line, uint8_t level, const wchar_t *fmt, ...) {
    wchar_t tmp[_LOGGER_MESSAGE_LIMIT_];
    va_list ap;
    struct logger_entryW *log;
    int n,m = 0;
    wchar_t *fname = wcsrchr(file, _CC_SLASH_C_);
    if (fname) {
        m = swprintf(tmp, _cc_countof(tmp), L"%s(%d)", fname + 1, line);
    }

    va_start(ap, fmt);
    n = vswprintf(tmp + m, _cc_countof(tmp) - m, fmt, ap);
    va_end(ap);

    if (n < 0) {
        return;
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
    memcpy(log->message, tmp, log->length * sizeof(wchar_t));
    log->message[log->length] = 0;
    log->level = level;
}

_CC_API_PUBLIC(void) _cc_loggerA_dump(_cc_loggerA_func_t pfun) {
    int r;
    struct logger_entryA *log;
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

static void header(uint8_t level, time_t timestamp) {
    struct tm tm_now;
    static const char  SYSLOG_LEVEL_CODE[_CC_LOG_LEVEL_DEBUG_ + 1] = {'G', 'A', 'C', 'E', 'W', 'N', 'I', 'D'};
    _cc_localtime(&timestamp, &tm_now);
    printf("<%c>%04d-%02d-%02d %02d:%02d:%02d ",
                                SYSLOG_LEVEL_CODE[level], 
                                tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday,
                                tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);
}
static void _alog (uint8_t level, time_t timestamp, const char_t *msg, int32_t length) {
    header(level, timestamp);
    printf("%.*s\n", length, msg);
}

static void _wlog(uint8_t level, time_t timestamp, const wchar_t *msg, int32_t length) {
    header(level, timestamp);
    wprintf(L"%.*s\n", msg);
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