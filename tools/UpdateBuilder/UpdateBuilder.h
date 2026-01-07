
#ifndef _C_UPDATE_BUILDER_H_INCLUDED_
#define _C_UPDATE_BUILDER_H_INCLUDED_

#include <libcc.h>
#include <libcc/widgets/sql.h>
#include <libcc/widgets/json.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

extern _cc_sql_delegate_t sql_delegator;

extern _cc_sds_t dest_directory;
extern _cc_sds_t source_directory;


int builder_reload(void);
int builder_updated(void);

_cc_sql_t* open_sqlite3(void);
void close_sqlite3(_cc_sql_t *sql);
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif
