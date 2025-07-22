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
#ifndef _C_CC_LOGGER_H_INCLUDED_
#define _C_CC_LOGGER_H_INCLUDED_

#include "endian.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

//Facility
enum {
    _CC_LOG_FACILITY_KERN_        = 0,  //Kernel messages
    _CC_LOG_FACILITY_USER_        = 1,  //User-level messages
    _CC_LOG_FACILITY_MAIL_        = 2,  //Mail system
    _CC_LOG_FACILITY_DAEMON_      = 3,  //System daemons
    _CC_LOG_FACILITY_AUTH_        = 4,  //Security/authentication messages
    _CC_LOG_FACILITY_SYSLOG_      = 5,  //Messages generated internally by syslogd
    _CC_LOG_FACILITY_LPR_         = 6,  //Line printer subsystem
    _CC_LOG_FACILITY_NEWS_        = 7,  //Network news subsystem
    _CC_LOG_FACILITY_UUCP_        = 8,  //UUCP subsystem
    _CC_LOG_FACILITY_CRON_        = 9,  //Clock daemon
    _CC_LOG_FACILITY_AUTHPRIV_    = 10, //Security/authentication messages
    _CC_LOG_FACILITY_FTP_         = 11, //FTP daemon
    _CC_LOG_FACILITY_NTP_         = 12, //NTP subsystem
    _CC_LOG_FACILITY_SECURITY_    = 13, //Log audit
    _CC_LOG_FACILITY_ALERT_       = 14, //Log alert
    //SOLARIS - CRON,                   //Scheduling daemon
    //LOCAL0 - LOCAL7,                  //Locally-used facilities
    _CC_LOG_FACILITY_LOCAL7_      = 23  //Memory tracking
};
//Severity Level
enum {
    _CC_LOG_LEVEL_EMERG_      = 0,      //System is unusable
    _CC_LOG_LEVEL_ALERT_      = 1,      //Action must be taken immediately
    _CC_LOG_LEVEL_CRIT_       = 2,      //Critical
    _CC_LOG_LEVEL_ERROR_      = 3,      //Error
    _CC_LOG_LEVEL_WARNING_    = 4,      //Warning 
    _CC_LOG_LEVEL_NOTICE_     = 5,      //Normal but significant condition
    _CC_LOG_LEVEL_INFO_       = 6,      //Informational messages
    _CC_LOG_LEVEL_DEBUG_      = 7       //Debug-level messages
};

#define _CC_SYSLOG_PORT_        514
#define _CC_SYSLOG_VERSIOV_     1
//RFC 3164
//<PRI>TIMESTAMP HOSTNAME TAG MSG

//RFC 5424
//TIMESTAMP ISO 8601  2023-12-12T12:34:56Z
//<PRI>VERSION TIMESTAMP HOSTNAME APP-NAME PROCID MSGID STRUCTURED-DATA MSG

//PRI = Facility * 8 + Severity
#define _CC_LOGGER_PRI(FACILITY,LEVEL) (((FACILITY) << 3) | ((LEVEL) & 0x7))
/**/
_CC_API_PUBLIC(void) _cc_logger_open_syslog(tchar_t *app_name, const tchar_t *ip, const uint16_t port);
/**/
_CC_API_PUBLIC(void) _cc_loggerW_syslog(byte_t pri, const wchar_t* msg, size_t length);
/**/
_CC_API_PUBLIC(void) _cc_loggerA_syslog(byte_t pri, const char_t* msg, size_t length);
/**/
_CC_API_PUBLIC(void) _cc_syslog(const byte_t* msg, size_t length);

/**/
_CC_API_PUBLIC(void) _cc_loggerA_vformat(byte_t level, const char_t* fmt, va_list arg);
/**/
_CC_API_PUBLIC(void) _cc_loggerA_format(byte_t level, const char_t* fmt, ...);
/**/
_CC_API_PUBLIC(void) _cc_loggerA(byte_t level, const char_t* str);
/**/
_CC_API_PUBLIC(void) _cc_loggerW_format_args(byte_t level, const wchar_t* fmt, va_list arg);
/**/
_CC_API_PUBLIC(void) _cc_loggerW_format(byte_t level, const wchar_t* fmt, ...);
/**/
_CC_API_PUBLIC(void) _cc_loggerW(byte_t level, const wchar_t* str);

#ifdef _CC_MSVC_
    /**/
    #define _cc_loggerW_debug(FMT, ...) \
        _cc_loggerW_format(_CC_LOG_LEVEL_DEBUG_, FMT, ##__VA_ARGS__)
    #define _cc_loggerA_debug(FMT, ...) \
        _cc_loggerA_format(_CC_LOG_LEVEL_DEBUG_, FMT, ##__VA_ARGS__)

    #define _cc_loggerW_info(FMT, ...) \
        _cc_loggerW_format(_CC_LOG_LEVEL_INFO_, FMT, ##__VA_ARGS__)
    #define _cc_loggerA_info(FMT, ...) \
        _cc_loggerA_format(_CC_LOG_LEVEL_INFO_, FMT, ##__VA_ARGS__)

    #define _cc_loggerW_warin(FMT, ...) \
        _cc_loggerW_format(_CC_LOG_LEVEL_WARNING_, FMT, ##__VA_ARGS__)
    #define _cc_loggerA_warin(FMT, ...) \
        _cc_loggerA_format(_CC_LOG_LEVEL_WARNING_, FMT, ##__VA_ARGS__)

    #define _cc_loggerW_error(FMT, ...) \
        _cc_loggerW_format(_CC_LOG_LEVEL_ERROR_, FMT, ##__VA_ARGS__)
    #define _cc_loggerA_error(FMT, ...) \
        _cc_loggerA_format(_CC_LOG_LEVEL_ERROR_, FMT, ##__VA_ARGS__)
#else
    #define _cc_loggerW_debug(FMT, ARGS...) \
        _cc_loggerW_format(_CC_LOG_LEVEL_DEBUG_, FMT, ##ARGS)
    #define _cc_loggerA_debug(FMT, ARGS...) \
        _cc_loggerA_format(_CC_LOG_LEVEL_DEBUG_, FMT, ##ARGS)

    #define _cc_loggerW_info(FMT, ARGS...) \
        _cc_loggerW_format(_CC_LOG_LEVEL_INFO_, FMT, ##ARGS)
    #define _cc_loggerA_info(FMT, ARGS...) \
        _cc_loggerA_format(_CC_LOG_LEVEL_INFO_, FMT, ##ARGS)

    #define _cc_loggerW_warin(FMT, ARGS...) \
        _cc_loggerW_format(_CC_LOG_LEVEL_WARNING_, FMT, ##ARGS)
    #define _cc_loggerA_warin(FMT, ARGS...) \
        _cc_loggerA_format(_CC_LOG_LEVEL_WARNING_, FMT, ##ARGS)

    #define _cc_loggerW_error(FMT, ARGS...) \
        _cc_loggerW_format(_CC_LOG_LEVEL_ERROR_, FMT, ##ARGS)
    #define _cc_loggerA_error(FMT, ARGS...) \
        _cc_loggerA_format(_CC_LOG_LEVEL_ERROR_, FMT, ##ARGS)
#endif

/**/
#ifdef _CC_UNICODE_
#define _cc_logger_format _cc_loggerW_format
#define _cc_logger _cc_loggerW
#define _cc_logger_warin _cc_loggerW_warin
#define _cc_logger_debug _cc_loggerW_debug
#define _cc_logger_info _cc_loggerW_info
#define _cc_logger_error _cc_loggerW_error
#define _cc_logger_syslog _cc_loggerW_syslog
#else
#define _cc_logger_format _cc_loggerA_format
#define _cc_logger _cc_loggerA
#define _cc_logger_warin _cc_loggerA_warin
#define _cc_logger_debug _cc_loggerA_debug
#define _cc_logger_info _cc_loggerA_info
#define _cc_logger_error _cc_loggerA_error
#define _cc_logger_syslog _cc_loggerA_syslog
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_LOGGER_H_INCLUDED_*/
