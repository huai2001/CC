
#ifndef _C_UDPSYSLOG_H_INCLUDED_
#define _C_UDPSYSLOG_H_INCLUDED_

#include <libcc.h>
#include <libcc/widgets/sql.h>
#include <libcc/widgets/event.h>

#if defined(_CC_UDPSYSLOG_EXPORT_SHARED_LIBRARY_)
    #define _CC_UDPSYSLOG_API(t) _CC_API_EXPORT_ t
#else
    #define _CC_UDPSYSLOG_API(t) t
#endif

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t version;
    uint8_t priority;
    time_t timestamp;
    int32_t pid;
    int32_t mid;
    _cc_String_t host;
    _cc_String_t app;
    _cc_String_t sd;
    _cc_String_t msg;
} _syslog_t;


extern tchar_t currentPath[_CC_MAX_PATH_];
extern _cc_sql_delegate_t delegate;

void openSQLite3(void);
void closeSQLit3(void);

void sqlite3_syslog(_syslog_t *syslog);
void sqlite3_exception_syslog(const tchar_t *msg, size_t length);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif
