#include <stdio.h>
#include "UDPSyslog.h"


_cc_sql_delegate_t delegator;
_cc_sql_t *defaultSQL = NULL;

const _cc_string_t createLogsTable = _cc_string("CREATE TABLE IF NOT EXISTS logs (" \
        ("id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL," \
        "facility   INTEGER         NOT NULL," \
        "severity   INTEGER         NOT NULL," \
        "host       VARCHAR(64)     DEFAULT ('-')," \
        "app        VARCHAR(64)     DEFAULT ('-')," \
        "date       INTEGER         DEFAULT (0)," \
        "pid        INTEGER         DEFAULT (0)," \
        "mid        INTEGER         DEFAULT (0)," \
        "sd         TEXT            DEFAULT ('-')," \
        "msg        TEXT            NOT NULL," \
        "create_date timestamp NOT NULL DEFAULT(DATETIME('now','localtime'))" \
    ")");
const _cc_string_t createExceptionTable = _cc_string("CREATE TABLE IF NOT EXISTS exception_logs ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
        "msg        TEXT            NOT NULL,"
        "create_date timestamp NOT NULL DEFAULT(DATETIME('now','localtime'))"
        ")");

void openSQLite3(void) {
    tchar_t fileSQLite[_CC_MAX_PATH_ * 2];
    bool_t isCreateTabel = false;

    if (defaultSQL) {
        return;
    }

    _cc_init_sqlite(&delegator);

    _sntprintf(fileSQLite,_cc_countof(fileSQLite), _T("SQLITE://127.0.0.1/%s/logs.db"), currentPath);
    if (_taccess(fileSQLite + sizeof(_T("SQLITE://127.0.0.1/")) - 1, _CC_ACCESS_F_) == -1) {
        isCreateTabel = true;
    }

    defaultSQL = delegator.connect(fileSQLite);
    if (defaultSQL == NULL) {
        return;
    }
    
    if (isCreateTabel) {
        delegator.execute(defaultSQL, createLogsTable, false);
        delegator.execute(defaultSQL, createExceptionTable, false);
    }
}

void closeSQLite3(void) {
    delegator.disconnect(defaultSQL);
    defaultSQL = NULL;
}

void sqlite3_syslog(_syslog_t *syslog) {
    _cc_sql_result_t *result = NULL;
    tchar_t *v = _T("-");
    uint8_t facility = _CC_SYSLOG_FACILITY(syslog->priority);
    uint8_t severity = _CC_SYSLOG_SEVERITY(syslog->priority);
    _cc_string_t sql;

    if (facility == 31) {
        _cc_string_set(sql,"DELETE FROM `logs` WHERE `host`=?");
        if (delegator.execute(defaultSQL, &sql, &result)) {
            delegator.bind(result, 0, syslog->host.data, syslog->host.length, _CC_SQL_TYPE_STRING_);
            delegator.step(defaultSQL, result);
            delegator.free_result(defaultSQL, result);
        }
        return;
    }

    if (syslog->host.length == 0) {
        syslog->host.data = v;
        syslog->host.length = 1;
    }
    if (syslog->app.length == 0) {
        syslog->app.data = v;
        syslog->app.length = 1;
    }
    if (syslog->sd.length == 0) {
        syslog->sd.data = v;
        syslog->sd.length = 1;
    }
    if (syslog->msg.length == 0) {
        syslog->msg.data = v;
        syslog->msg.length = 1;
    }

    _cc_string_set(sql,"INSERT INTO `logs` (`facility`, `severity`, `host`, `app`, `pid`, `mid`, `date`, `sd`, `msg`) VALUES "\
                                  "( ?, ?, ?, ?, ?, ?, ?, ?, ?);");
    if (delegator.execute(defaultSQL, &sql, &result)) {
        uint8_t i = 0;
        //delegator.reset(defaultSQL, result);
        delegator.bind(result, i++, &facility, sizeof(facility), _CC_SQL_TYPE_INT8_);
        delegator.bind(result, i++, &severity, sizeof(severity), _CC_SQL_TYPE_INT8_);
        delegator.bind(result, i++, syslog->host.data, syslog->host.length, _CC_SQL_TYPE_STRING_);
        delegator.bind(result, i++, syslog->app.data, syslog->app.length, _CC_SQL_TYPE_STRING_);

        delegator.bind(result, i++, &syslog->pid, sizeof(int32_t), _CC_SQL_TYPE_INT32_);
        delegator.bind(result, i++, &syslog->mid, sizeof(int32_t), _CC_SQL_TYPE_INT32_);
        delegator.bind(result, i++, &syslog->timestamp, sizeof(time_t), _CC_SQL_TYPE_INT64_);
        delegator.bind(result, i++, syslog->sd.data, syslog->sd.length, _CC_SQL_TYPE_STRING_);
        delegator.bind(result, i++, syslog->msg.data, syslog->msg.length, _CC_SQL_TYPE_STRING_);
        delegator.step(defaultSQL, result);
        delegator.free_result(defaultSQL, result);
    }
}

void sqlite3_exception_syslog(const tchar_t *msg, size_t length) {
    _cc_sql_result_t *result = NULL;
    _cc_string_t sql = _cc_string("INSERT INTO `exception_logs` (`msg`) VALUES ( ? );");
    if (delegator.execute(defaultSQL, &sql, &result)) {
        delegator.bind(result, 0, msg, length, _CC_SQL_TYPE_STRING_);
        delegator.step(defaultSQL, result);
        delegator.free_result(defaultSQL, result);
    }
}