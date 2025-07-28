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
#ifndef _C_CC_SYSLOG_H_INCLUDED_
#define _C_CC_SYSLOG_H_INCLUDED_

#include "endian.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define _CC_SYSLOG_PORT_ 514
#define _CC_SYSLOG_VERSIOV_ 1

// PRI = Facility * 8 + Severity
#define _CC_SYSLOG_PRI(F, L) (((F) << 3) | ((L) & 0x7))

#define _CC_SYSLOG_SEVERITY(n) ((n) & 7)
#define _CC_SYSLOG_FACILITY(n) (((n) >> 3) & 0xFF)

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

//RFC 3164
//<PRI>TIMESTAMP HOSTNAME TAG MSG

//RFC 5424
//TIMESTAMP ISO 8601  2023-12-12T12:34:56Z
//<PRI>VERSION TIMESTAMP HOSTNAME APP-NAME PROCID MSGID STRUCTURED-DATA MSG

/**/
_CC_API_PUBLIC(void) _cc_open_syslog(uint8_t facility, const tchar_t *app_name, const tchar_t *ip, const uint16_t port);
/**/
_CC_API_PUBLIC(void) _cc_close_syslog(void);
/**/
_CC_API_PUBLIC(void) _cc_syslogW(uint8_t level, const wchar_t* msg, size_t length);
/**/
_CC_API_PUBLIC(void) _cc_syslogA(uint8_t level, const char_t* msg, size_t length);
/**/
_CC_API_PUBLIC(size_t) _cc_syslog_header(uint8_t pri, tchar_t *buffer, size_t buffer_length);
/**/
_CC_API_PUBLIC(void) _cc_syslog_send(const uint8_t *msg, size_t length);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_SYSLOG_H_INCLUDED_*/
