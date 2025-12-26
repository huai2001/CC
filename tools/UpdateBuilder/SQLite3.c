#include <stdio.h>
#include "UpdateBuilder.h"

_cc_sql_delegate_t sqldelegate;
_cc_sql_t *sql_default = NULL;

const tchar_t *createTable = "CREATE TABLE IF NOT EXISTS FileList (" \
        "ID INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL," \
        "Name VARCHAR(45) NOT NULL," \
        "CheckMD5 VARCHAR(33) NOT NULL," \
        "Compress VARCHAR(45) NOT NULL," \
        "CompressSize INTEGER NOT NULL," \
        "Size INTEGER NOT NULL," \
        "Path VARCHAR(256) NOT NULL," \
        "LastUpdate timestamp NOT NULL DEFAULT(DATETIME('now','localtime'))" \
    ")";


_cc_sql_t* openSQLite3(void) {
    _cc_sql_t *sql = NULL;
    tchar_t path[_CC_MAX_PATH_];
    tchar_t sqliteFile[_CC_MAX_PATH_ * 2];

    if (sql_default) {
        return sql_default;
    }
    
    _cc_get_base_path(path, _cc_countof(path));
    _sntprintf(sqliteFile,_cc_countof(sqliteFile), _T("SQLITE:///%s/UpdateBuilder.db"), path);

    sql = sqldelegate.connect(sqliteFile);
    if (sql == NULL) {
        _cc_logger_debug("Update SQL is null");
        return NULL;
    }

    sqldelegate.execute(sql, createTable, false);

    sql_default = sql;
    return sql;
}

void closeSQLite3(_cc_sql_t *sql) {
    if (sql) {
        sqldelegate.disconnect(sql);
        sql_default = NULL;
    }
}