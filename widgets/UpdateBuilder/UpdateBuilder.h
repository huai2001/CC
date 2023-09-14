
#ifndef _C_UPDATE_BUILDER_H_INCLUDED_
#define _C_UPDATE_BUILDER_H_INCLUDED_

#include <libcc.h>
#include <cc/db/sql.h>
#include <cc/json/json.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

extern _cc_sql_driver_t sqlDriver;

extern int32_t updateDirectoryLen;
extern tchar_t updateDirectory[_CC_MAX_PATH_];

extern int32_t sourceDirectoryLen;
extern tchar_t sourceDirectory[_CC_MAX_PATH_];


int builder_ReloadList(void);
int builder_UpdateList(void);

_cc_sql_t* openSQLite3(void);
void closeSQLit3(_cc_sql_t *sql);
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif
