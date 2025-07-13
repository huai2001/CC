#include <stdio.h>
#include "UpdateBuilder.h"

_cc_sql_delegate_t sqldelegate;
_cc_sql_t *sql_default = nullptr;

const tchar_t *createTable = "CREATE TABLE IF NOT EXISTS FileList (" \
        "ID INTEGER PRIMARY KEY AUTOINCREMENT NOT nullptr," \
        "Name VARCHAR(45) NOT nullptr," \
        "CheckMD5 VARCHAR(33) NOT nullptr," \
        "Compress VARCHAR(45) NOT nullptr," \
        "CompressSize INTEGER NOT nullptr," \
        "Size INTEGER NOT nullptr," \
        "Path TEXT NOT nullptr," \
        "LastUpdate timestamp NOT nullptr DEFAULT(DATETIME('now','localtime'))" \
    ")";


_cc_sql_t* openSQLite3(void) {
    _cc_sql_t *sql = nullptr;
    tchar_t path[_CC_MAX_PATH_];
    tchar_t sqliteFile[_CC_MAX_PATH_ * 2];

    if (sql_default) {
        return sql_default;
    }
    
    _cc_get_base_path(path, _cc_countof(path));
    _sntprintf(sqliteFile,_cc_countof(sqliteFile), _T("SQLITE://x/%s/UpdateBuilder.db"), path);

    sql = sqldelegate.connect(sqliteFile);
    if (sql == nullptr) {
        _cc_logger_debug("Update SQL is nullptr");
    }

    sqldelegate.execute(sql, createTable, false);

    sql_default = sql;
    return sql;
}

void closeSQLit3(_cc_sql_t *sql) {
    if (sql) {
        sqldelegate.disconnect(sql);
        sql_default = nullptr;
    }
}