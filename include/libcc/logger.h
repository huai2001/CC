#ifndef _C_CC_LOGGER_H_INCLUDED_
#define _C_CC_LOGGER_H_INCLUDED_

#include "cores.h"
#include <time.h>

#define _CC_SYSLOG_PORT_ 514
#define _CC_SYSLOG_VERSIOV_ 1
#define _CC_SYSLOG_RFC5424_ 0

// PRI = Facility * 8 + Severity
#define _CC_SYSLOG_PRI(F, L) (((F) << 3) | ((L) & 0x7))

#define _CC_SYSLOG_SEVERITY(n) ((n) & 7)
#define _CC_SYSLOG_FACILITY(n) (((n) >> 3) & 0xFF)

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

// Facility
enum {
    _CC_LOG_FACILITY_KERN_ = 0, // Kernel messages
    _CC_LOG_FACILITY_USER_,     // User-level messages
    _CC_LOG_FACILITY_MAIL_,     // Mail system
    _CC_LOG_FACILITY_DAEMON_,   // System daemons
    _CC_LOG_FACILITY_AUTH_,     // Security/authentication messages
    _CC_LOG_FACILITY_SYSLOG_,   // Messages generated internally by syslogd
    _CC_LOG_FACILITY_LPR_,      // Line printer subsystem
    _CC_LOG_FACILITY_NEWS_,     // Network news subsystem
    _CC_LOG_FACILITY_UUCP_,     // UUCP subsystem
    _CC_LOG_FACILITY_CRON_,     // Clock daemon
    _CC_LOG_FACILITY_AUTHPRIV_, // Security/authentication messages
    _CC_LOG_FACILITY_FTP_,      // FTP daemon
    _CC_LOG_FACILITY_NTP_,      // NTP subsystem

    _CC_LOG_FACILITY_SECURITY_, // Log audit
    _CC_LOG_FACILITY_CONSOLE_,  // Log console

    // //Locally-used facilities
    _CC_LOG_FACILITY_LOCAL0_ = 16,
    _CC_LOG_FACILITY_LOCAL1_,
    _CC_LOG_FACILITY_LOCAL2_,
    _CC_LOG_FACILITY_LOCAL3_,
    _CC_LOG_FACILITY_LOCAL4_,
    _CC_LOG_FACILITY_LOCAL5_,
    _CC_LOG_FACILITY_LOCAL6_,
    _CC_LOG_FACILITY_LOCAL7_ = 23 // Memory tracking
};

// Severity Level
enum {
    _CC_LOG_LEVEL_EMERG_ = 0, // System is unusable
    _CC_LOG_LEVEL_ALERT_,     // Action must be taken immediately
    _CC_LOG_LEVEL_CRIT_,      // Critical
    _CC_LOG_LEVEL_ERROR_,     // Error
    _CC_LOG_LEVEL_WARNING_,   // Warning
    _CC_LOG_LEVEL_NOTICE_,    // Normal but significant condition
    _CC_LOG_LEVEL_INFO_,      // Informational messages
    _CC_LOG_LEVEL_DEBUG_      // Debug-level messages
};

typedef void (*_cc_loggerA_func_t)(uint8_t level, time_t timestamp, const char_t *msg, int32_t length);
typedef void (*_cc_loggerW_func_t)(uint8_t level, time_t timestamp, const wchar_t *msg, int32_t length);

/**/
void __install_logger(void);
/**/
void __uninstall_logger(void);
/**/
_CC_API_PUBLIC(void) _cc_loggerW_dump(_cc_loggerW_func_t pfun);
/**/
_CC_API_PUBLIC(void) _cc_loggerA_dump(_cc_loggerA_func_t pfun);
/**/
_CC_API_PUBLIC(void) _cc_logger_dump(void);
/**/
#ifndef _CL
    #define _CL(x) __CL(x)
    #define __CL(x) L##x
#endif

/**/
_CC_API_PUBLIC(void) _cc_loggerA(const char_t *file, int line, uint8_t level, const char_t* fmt, ...);
/**/
_CC_API_PUBLIC(void) _cc_loggerW(const wchar_t *file, int line, uint8_t level, const wchar_t* fmt, ...);

#ifdef __CC_MSVC__
    #define _cc_loggerW_alert(FMT, ...) \
        _cc_loggerW(_CL(_CC_FILE_), _CC_LINE_, _CC_LOG_LEVEL_ALERT_, _CL(FMT), ##__VA_ARGS__)
    #define _cc_loggerA_alert(FMT, ...) \
        _cc_loggerA(_CC_FILE_, _CC_LINE_, _CC_LOG_LEVEL_ALERT_, FMT, ##__VA_ARGS__)

    #define _cc_loggerW_debug(FMT, ...) \
        _cc_loggerW(_CL(_CC_FILE_), _CC_LINE_, _CC_LOG_LEVEL_DEBUG_, _CL(FMT), ##__VA_ARGS__)
    #define _cc_loggerA_debug(FMT, ...) \
        _cc_loggerA(_CC_FILE_, _CC_LINE_, _CC_LOG_LEVEL_DEBUG_, FMT, ##__VA_ARGS__)

    #define _cc_loggerW_info(FMT, ...) \
        _cc_loggerW(_CL(_CC_FILE_), _CC_LINE_, _CC_LOG_LEVEL_INFO_, _CL(FMT), ##__VA_ARGS__)
    #define _cc_loggerA_info(FMT, ...) \
        _cc_loggerA(_CC_FILE_, _CC_LINE_, _CC_LOG_LEVEL_INFO_, FMT, ##__VA_ARGS__)

    #define _cc_loggerW_warin(FMT, ...) \
        _cc_loggerW(_CL(_CC_FILE_), _CC_LINE_, _CC_LOG_LEVEL_WARNING_, _CL(FMT), ##__VA_ARGS__)
    #define _cc_loggerA_warin(FMT, ...) \
        _cc_loggerA(_CC_FILE_, _CC_LINE_, _CC_LOG_LEVEL_WARNING_, FMT, ##__VA_ARGS__)

    #define _cc_loggerW_error(FMT, ...) \
        _cc_loggerW(_CL(_CC_FILE_), _CC_LINE_, _CC_LOG_LEVEL_ERROR_, _CL(FMT), ##__VA_ARGS__)
    #define _cc_loggerA_error(FMT, ...) \
        _cc_loggerA(_CC_FILE_, _CC_LINE_, _CC_LOG_LEVEL_ERROR_, FMT, ##__VA_ARGS__)
#else
    #define _cc_loggerW_alert(FMT, ARGS...) \
        _cc_loggerW(_CL(_CC_FILE_), _CC_LINE_, _CC_LOG_LEVEL_ALERT_, _CL(FMT), ##ARGS)
    #define _cc_loggerA_alert(FMT, ARGS...) \
        _cc_loggerA(_CC_FILE_, _CC_LINE_, _CC_LOG_LEVEL_ALERT_, FMT, ##ARGS)

    #define _cc_loggerW_debug(FMT, ARGS...) \
        _cc_loggerW(_CL(_CC_FILE_), _CC_LINE_, _CC_LOG_LEVEL_DEBUG_, _CL(FMT), ##ARGS)
    #define _cc_loggerA_debug(FMT, ARGS...) \
        _cc_loggerA(_CC_FILE_, _CC_LINE_, _CC_LOG_LEVEL_DEBUG_, FMT, ##ARGS)

    #define _cc_loggerW_info(FMT, ARGS...) \
        _cc_loggerW(_CL(_CC_FILE_), _CC_LINE_, _CC_LOG_LEVEL_INFO_, _CL(FMT), ##ARGS)
    #define _cc_loggerA_info(FMT, ARGS...) \
        _cc_loggerA(_CC_FILE_, _CC_LINE_, _CC_LOG_LEVEL_INFO_, FMT, ##ARGS)

    #define _cc_loggerW_warin(FMT, ARGS...) \
        _cc_loggerW(_CL(_CC_FILE_), _CC_LINE_, _CC_LOG_LEVEL_WARNING_, _CL(FMT), ##ARGS)
    #define _cc_loggerA_warin(FMT, ARGS...) \
        _cc_loggerA(_CC_FILE_, _CC_LINE_, _CC_LOG_LEVEL_WARNING_, FMT, ##ARGS)

    #define _cc_loggerW_error(FMT, ARGS...) \
        _cc_loggerW(_CL(_CC_FILE_), _CC_LINE_, _CC_LOG_LEVEL_ERROR_, _CL(FMT), ##ARGS)
    #define _cc_loggerA_error(FMT, ARGS...) \
        _cc_loggerA(_CC_FILE_, _CC_LINE_, _CC_LOG_LEVEL_ERROR_, FMT, ##ARGS)
#endif

/**/
#ifdef _CC_UNICODE_
    #ifdef __CC_MSVC__
        #define _cc_logger(LEVEL, FMT, ...) _cc_loggerW(_CL(_CC_FILE_), _CC_LINE_, LEVEL, _CL(FMT), ##__VA_ARGS__)
    #else
        #define _cc_logger(LEVEL, FMT, ARGS...) _cc_loggerW(_CL(_CC_FILE_), _CC_LINE_, LEVEL, _CL(FMT), ##ARGS)
    #endif

    #define _cc_logger_warin _cc_loggerW_warin
    #define _cc_logger_debug _cc_loggerW_debug
    #define _cc_logger_info _cc_loggerW_info
    #define _cc_logger_error _cc_loggerW_error
    #define _cc_logger_alert _cc_loggerW_alert
#else
    #ifdef __CC_MSVC__
        #define _cc_logger(LEVEL, FMT, ...) _cc_loggerA(_CC_FILE_, _CC_LINE_, LEVEL, FMT, ##__VA_ARGS__)
    #else
        #define _cc_logger(LEVEL, FMT, ARGS...) _cc_loggerA(_CC_FILE_, _CC_LINE_, LEVEL, FMT, ##ARGS)
    #endif

    #define _cc_logger_warin _cc_loggerA_warin
    #define _cc_logger_debug _cc_loggerA_debug
    #define _cc_logger_info _cc_loggerA_info
    #define _cc_logger_error _cc_loggerA_error
    #define _cc_logger_alert _cc_loggerA_alert
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_LOGGER_H_INCLUDED_*/
