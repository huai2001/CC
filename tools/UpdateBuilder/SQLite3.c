#include <stdio.h>
#include "UpdateBuilder.h"

_cc_sql_delegate_t sql_delegator;
_cc_sql_t *sql_default = NULL;


_cc_sql_t* open_sqlite3(void) {
    _cc_sql_t *sql = NULL;
    tchar_t path[_CC_MAX_PATH_];
    tchar_t sqlite_file[_CC_MAX_PATH_ * 2];

    if (sql_default) {
        return sql_default;
    }
    
    _cc_get_base_path(path, _cc_countof(path));
    _sntprintf(sqlite_file,_cc_countof(sqlite_file), _T("SQLITE:///%s/UpdateBuilder.db"), path);

    sql = sql_delegator.connect(sqlite_file);
    if (sql == NULL) {
        _cc_logger_debug("Update SQL is null");
        return NULL;
    }

    sql_delegator.execute(sql, _CC_SQL("CREATE TABLE IF NOT EXISTS FileList (" \
            "ID INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL," \
            "Name VARCHAR(45) NOT NULL," \
            "CheckMD5 VARCHAR(33) NOT NULL," \
            "Compress VARCHAR(45) NOT NULL," \
            "CompressSize INTEGER NOT NULL," \
            "Size INTEGER NOT NULL," \
            "Path VARCHAR(256) NOT NULL," \
            "LastUpdate timestamp NOT NULL DEFAULT(DATETIME('now','localtime'))" \
        ")"), false);

    sql_default = sql;
    return sql;
}

void close_sqlite3(_cc_sql_t *sql) {
    if (sql) {
        sql_delegator.disconnect(sql);
        sql_default = NULL;
    }
}