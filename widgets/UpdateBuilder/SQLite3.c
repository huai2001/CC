#include <stdio.h>
#include "UpdateBuilder.h"

_cc_sql_driver_t sqlDriver;
_cc_sql_t *sql_default = NULL;

const tchar_t *createTable = "create table if not exists FileList (" \
        "ID INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL," \
        "Name VARCHAR(45) NOT NULL," \
        "CheckMD5 VARCHAR(33) NOT NULL," \
        "Compress VARCHAR(45) NOT NULL," \
        "CompressSize INTEGER NOT NULL," \
        "Size INTEGER NOT NULL," \
        "Path TEXT NOT NULL," \
        "LastUpdate timestamp NOT NULL default(datetime('now','localtime'))" \
    ")";


_cc_sql_t* openSQLite3(void) {
    _cc_sql_t *sql = NULL;
    tchar_t directory[_CC_MAX_PATH_];
    tchar_t sqliteFile[_CC_MAX_PATH_];

    if (sql_default) {
        return sql_default;
    }

    _cc_get_module_directory(_T("UpdateBuilder.db"), directory, _cc_countof(directory));
    
    _sntprintf(sqliteFile,_cc_countof(sqliteFile), _T("SQLITE://127.0.0.1/%s"), directory);

    sql = sqlDriver.connect(sqliteFile);
    if (sql == NULL) {
        _cc_logger_debug("Update SQL is NULL");
    }

    sqlDriver.execute(sql, createTable, false);

    sql_default = sql;
    return sql;
}

void closeSQLit3(_cc_sql_t *sql) {
    if (sql) {
        sqlDriver.disconnect(sql);
        sql_default = NULL;
    }
}