/*
 * Copyright libcc.cn@gmail.com. and other libCC contributors.
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
#include <libcc/alloc.h>
#include <libcc/atomic.h>
#include <libcc/time.h>
#include <libcc/types.h>
#include <libcc/widgets/db/sql.h>
#include <time.h>

#ifdef __CC_WINDOWS__
#include <sqlext.h>
#include <windows.h>
#elif defined(_CC_ENABLE_UNIXODBC_)
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
#endif

static _cc_atomic32_t sql_started_refcount = 0;
static SQLHENV hEnv = SQL_NULL_HENV;

struct _cc_sql {
    SQLHDBC hDBC;
    /* 0 for manual commit */
    bool_t auto_commit;
};

struct _cc_sql_result {
    SQLHSTMT hSTMT;
};
/*
** Fails with error message from ODBC
** Inputs:
**   type: type of handle used in operation
**   handle: handle used in operation
*/
/*
_CC_API_PRIVATE(void) failODBC(const SQLSMALLINT type,
                                const SQLHANDLE handle) {
    SQLTCHAR State[6];
    SQLINTEGER NativeError;
    SQLSMALLINT MsgSize, i;
    SQLRETURN res;
    SQLTCHAR Msg[SQL_MAX_MESSAGE_LENGTH];

    i = 1;
    do {
        res = SQLGetDiagRec(type, handle, i, State, &NativeError, Msg,
            sizeof(Msg), &MsgSize);
        i++;
        _tprintf(_T("%s\n"),Msg);
    } while (res != SQL_NO_DATA);
}*/

_CC_API_PRIVATE(int) sql_error(SQLRETURN a) {
    return (a != SQL_SUCCESS) && (a != SQL_SUCCESS_WITH_INFO) && (a != SQL_NO_DATA);
}

_CC_API_PRIVATE(bool_t) checkErrorCode(SQLRETURN rc, SQLHDBC hDBC, SQLHSTMT hSTMT, tchar_t *msg) {
#define MSG_LNG 512
    SQLTCHAR szSqlState[MSG_LNG]; /* SQL state string */
    SQLINTEGER pfNativeError;     /* Native error code */
    SQLTCHAR szErrorMsg[MSG_LNG]; /* Error msg text buffer pointer */
    SQLSMALLINT pcbErrorMsg;      /* Error msg text Available bytes */
    SQLRETURN res = SQL_SUCCESS;
    if (rc != SQL_SUCCESS && rc != SQL_NO_DATA_FOUND && rc != SQL_SUCCESS_WITH_INFO) {
        if (rc != SQL_SUCCESS_WITH_INFO) { /* It's not just a warning */
            _cc_logger(_CC_LOGGER_FLAGS_ERROR_, msg);
        }
        /*
         * Now see why the error/warning occurred
         */
        while (res == SQL_SUCCESS || res == SQL_SUCCESS_WITH_INFO) {
            res = SQLError(hEnv, hDBC, hSTMT, szSqlState, &pfNativeError, szErrorMsg, MSG_LNG, &pcbErrorMsg);
            switch (res) {
            case SQL_SUCCESS:
                _cc_logger(_CC_LOGGER_FLAGS_ERROR_, (const tchar_t *)szErrorMsg);
                _cc_logger_format(_CC_LOGGER_FLAGS_ERROR_,
                                  _T("ODBC Error/Warning = %s, TimesTen ")
                                  _T("Error/Warning = %d"),
                                  szErrorMsg, szSqlState, pfNativeError);
                break;
            case SQL_SUCCESS_WITH_INFO:
                _cc_logger(_CC_LOGGER_FLAGS_ERROR_, _T("Call to SQLError failed with return code ")
                                                    _T("of SQL_SUCCESS_WITH_INFO."));
                _cc_logger(_CC_LOGGER_FLAGS_ERROR_, _T("Need to increase size of message buffer."));
                break;
            case SQL_INVALID_HANDLE:
                _cc_logger(_CC_LOGGER_FLAGS_ERROR_, _T("Call to SQLError failed with return code ")
                                                    _T("of SQL_INVALID_HANDLE."));
                break;
            case SQL_ERROR:
                _cc_logger(_CC_LOGGER_FLAGS_ERROR_, _T("Call to SQLError failed with return code ")
                                                    _T("of SQL_ERROR."));
                break;
            case SQL_NO_DATA_FOUND:
                _cc_logger(_CC_LOGGER_FLAGS_ERROR_, _T("SQL_NO_DATA_FOUND"));
                break;
            default:
                _cc_logger(_CC_LOGGER_FLAGS_ERROR_, _T("Call to SQLError failed with return code ")
                                                    _T("of UNKNOW."));
                break;
            } /* switch */
        }     /* while */
        return false;
    }

    return true;
}

_CC_API_PRIVATE(bool_t) _init_sqlsvr(void) {
    int request = 0;

    if (_cc_unlikely(hEnv != SQL_NULL_HENV)) {
        return true;
    }

    /*Allocate the ODBC environment and save handle.*/
    request = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    if ( (request != SQL_SUCCESS_WITH_INFO) && (request != SQL_SUCCESS)) {  
      _cc_logger_error(_T("SQLAllocHandle(Env) Failed"));
      return false;
   }

    /*Notify ODBC that this is an ODBC 3.0 app.*/
    request = SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER *)SQL_OV_ODBC3, SQL_IS_INTEGER);
    if ( (request != SQL_SUCCESS_WITH_INFO) && (request != SQL_SUCCESS)) {  
        _cc_logger_error(_T("SQLSetEnvAttr(ODBC version) Failed"));
        SQLFreeEnv(hEnv);
        hEnv = SQL_NULL_HENV;
        return false;
   }
    return true;
}

_CC_API_PRIVATE(bool_t) _quit_sqlsvr(void) {
    if (_cc_likely(hEnv != SQL_NULL_HENV)) {
        SQLFreeEnv(hEnv);
        hEnv = SQL_NULL_HENV;
    }

    return true;
}

_CC_API_PRIVATE(_cc_sql_t*) _sqlsvr_connect(const tchar_t *sql_connection_string) {
    SQLTCHAR OutConnStr[1024] = {0};
    SQLSMALLINT OutConnStrLen;
    _cc_sql_t *ctx = nullptr;

    int request = SQL_SUCCESS;

    if (!_init_sqlsvr()) {
        return nullptr;
    }

    ctx = (_cc_sql_t *)_cc_malloc(sizeof(_cc_sql_t));
    ctx->auto_commit = true;
    ctx->hDBC = SQL_NULL_HDBC;
    /* allocate a connection handle*/
    request = SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &ctx->hDBC);

    if (checkErrorCode(request, ctx->hDBC, SQL_NULL_HSTMT, _T("Unable allocate connection handle.")) == false) {
        _cc_free(ctx);
        return nullptr;
    }

    SQLSetConnectAttr(ctx->hDBC, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

    request = SQLdelegateConnect(ctx->hDBC, nullptr, (SQLTCHAR *)sql_connection_string,
                               (SQLSMALLINT)_tcslen(sql_connection_string), OutConnStr,
                               (SQLSMALLINT)_cc_countof(OutConnStr), &OutConnStrLen, SQL_delegate_NOPROMPT);

    if (checkErrorCode(request, ctx->hDBC, SQL_NULL_HSTMT, _T("Error in connecting to the delegate.")) == false) {
        _cc_free(ctx);
        return nullptr;
    }

    _cc_atomic32_inc(&sql_started_refcount);

    return ctx;
}

_CC_API_PRIVATE(bool_t) _sqlsvr_disconnect(_cc_sql_t *ctx) {
    _cc_assert(ctx != nullptr);

    if (ctx->hDBC != SQL_NULL_HDBC) {
        /*disconnect, and if it fails, then issue a rollback for any pending
         * transaction (lurcher)*/
        if (SQLDisconnect(ctx->hDBC) == SQL_ERROR) {
            SQLTransact(nullptr, ctx->hDBC, SQL_ROLLBACK);
            SQLDisconnect(ctx->hDBC);
        }

        SQLFreeHandle(SQL_HANDLE_DESC, ctx->hDBC);
        SQLFreeConnect(ctx->hDBC);

        ctx->hDBC = SQL_NULL_HDBC;
    }

    if (_cc_atomic32_dec_ref(&sql_started_refcount)) {
        _quit_sqlsvr();
    }
    return true;
}

_CC_API_PRIVATE(bool_t) _sqlsvr_execute(_cc_sql_t *ctx, const tchar_t *sql_string, _cc_sql_result_t **result) {
    SQLHSTMT hSTMT = SQL_NULL_HSTMT;
    int request = SQL_SUCCESS;
    _cc_assert(ctx != nullptr && ctx->hDBC != SQL_NULL_HSTMT);

    request = SQLAllocStmt(ctx->hDBC, &hSTMT);
    if (checkErrorCode(request, ctx->hDBC, hSTMT, _T("Unable to allocate a statement handle.")) == false) {
        return false;
    }

    request = SQLPrepare(hSTMT, (SQLTCHAR *)sql_string, SQL_NTS);
    if (checkErrorCode(request, ctx->hDBC, hSTMT, _T("Unable to SQLPrepare.")) == false) {
        return false;
    }

    request = SQLExecute(hSTMT);
    if (request == SQL_NEED_DATA) {
        char buf[_CC_4K_BUFFER_SIZE_];
        int fp;
        SQLLEN nbytes;
        while (request == SQL_NEED_DATA) {
            request = SQLParamData(hSTMT, (SQLPOINTER *)&fp);

            if (request == SQL_NEED_DATA) {
                while ((nbytes = (SQLLEN)read(fp, &buf, _CC_4K_BUFFER_SIZE_)) > 0) {
                    SQLPutData(hSTMT, (void *)&buf, nbytes);
                }
            }
        }
    } else if (checkErrorCode(request, ctx->hDBC, hSTMT, _T("Unable to SQLExecute.")) == false) {
        return false;
    }

    if (_cc_unlikely(hSTMT == SQL_NULL_HSTMT)) {
        return false;
    }

    if (result) {
        *result = (_cc_sql_result_t *)_cc_malloc(sizeof(_cc_sql_result_t));
        (*result)->hSTMT = hSTMT;
        return true;
    }

    SQLCloseCursor(hSTMT);
    SQLFreeStmt(hSTMT, SQL_CLOSE);
    SQLFreeStmt(hSTMT, SQL_DROP);
    SQLFreeHandle(SQL_HANDLE_STMT, hSTMT);

    return true;
}

_CC_API_PRIVATE(bool_t) _sqlsvr_auto_commit(_cc_sql_t *ctx, bool_t is_auto_commit) {
    SQLRETURN rc = 0;
    _cc_assert(ctx != nullptr);

    if (is_auto_commit) {
        rc = SQLSetConnectAttr(ctx->hDBC, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
    } else {
        rc = SQLSetConnectAttr(ctx->hDBC, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0);
    }

    if (checkErrorCode(rc, ctx->hDBC, SQL_NULL_HSTMT, _T("Unable to SQL_ROLLBACK.")) == false) {
        return false;
    }
    return true;
}

_CC_API_PRIVATE(bool_t) _sqlsvr_begin_transaction(_cc_sql_t *ctx) {
    _cc_assert(ctx != nullptr);

    if (!ctx->auto_commit) {
        return true;
    }

    return _sqlsvr_auto_commit(ctx, false);
}

_CC_API_PRIVATE(bool_t) _sqlsvr_commit(_cc_sql_t *ctx) {
    SQLRETURN rc = 0;
    _cc_assert(ctx != nullptr);

    rc = SQLEndTran(SQL_HANDLE_DBC, ctx->hDBC, SQL_COMMIT);

    if (checkErrorCode(rc, ctx->hDBC, SQL_NULL_HSTMT, _T("Unable to SQL_COMMIT.")) == false) {
        return false;
    }

    return true;
}

_CC_API_PRIVATE(bool_t) _sqlsvr_rollback(_cc_sql_t *ctx) {
    SQLRETURN rc = 0;
    _cc_assert(ctx != nullptr);

    rc = SQLEndTran(SQL_HANDLE_DBC, ctx->hDBC, SQL_ROLLBACK);

    if (checkErrorCode(rc, ctx->hDBC, SQL_NULL_HSTMT, _T("Unable to SQL_ROLLBACK.")) == false) {
        return false;
    }

    return true;
}

_CC_API_PRIVATE(uint64_t) _sqlsvr_get_num_rows(_cc_sql_result_t *result) {
    SQLLEN numrows;
    SQLRETURN rc = 0;
    _cc_assert(result != nullptr && result->hSTMT != SQL_NULL_HSTMT);

    /* determine the number of result rows */
    rc = SQLRowCount(result->hSTMT, (SQLLEN *)&numrows);
    if (sql_error(rc)) {
        _cc_logger_error(_T("Unable to SQLRowCount."));
        return 0;
    }

    return (uint64_t)numrows;
}

_CC_API_PRIVATE(bool_t) _sqlsvr_fetch(_cc_sql_result_t *result) {
    _cc_assert(result != nullptr && result->hSTMT != SQL_NULL_HSTMT);
    return SQLFetch(result->hSTMT) != SQL_NO_DATA;
}

_CC_API_PRIVATE(int32_t) _sqlsvr_get_num_fields(_cc_sql_result_t *result) {
    SQLSMALLINT numcols;
    SQLRETURN rc = 0;
    _cc_assert(result != nullptr && result->hSTMT != SQL_NULL_HSTMT);

    /* determine the number of result columns */
    rc = SQLNumResultCols(result->hSTMT, &numcols);
    if (sql_error(rc)) {
        _cc_logger_error(_T("Unable to SQLNumResultCols."));
        return 0;
    }

    return numcols;
}

#if 0
/*
** Returns the name  for a SQL type.
*/
_CC_API_PRIVATE(const char *) _sqlsvr_types (const SQLSMALLINT type) {
    switch (type) {
        case SQL_UNKNOWN_TYPE:
        case SQL_CHAR:
        case SQL_VARCHAR: 
        case SQL_TYPE_DATE:
        case SQL_TYPE_TIME:
        case SQL_TYPE_TIMESTAMP: 
        case SQL_DATE:
        case SQL_INTERVAL:
        case SQL_TIMESTAMP: 
        case SQL_LONGVARCHAR:
        case SQL_WCHAR:
        case SQL_WVARCHAR:
        case SQL_WLONGVARCHAR:
            return "string";
        case SQL_BIGINT:
        case SQL_TINYINT: 
        case SQL_INTEGER:
        case SQL_SMALLINT: 
            return "integer";
        case SQL_NUMERIC:
        case SQL_DECIMAL: 
        case SQL_FLOAT:
        case SQL_REAL:
        case SQL_DOUBLE:
            return "number";
        case SQL_BINARY:
        case SQL_VARBINARY:
        case SQL_LONGVARBINARY:
            return "binary";    /* !!!!!! nao seria string? */
        case SQL_BIT:
            return "boolean";
        default:
            assert(0);
            return nullptr;
    }
}
_CC_API_PRIVATE(bool_t) _sqlsvr_get_field_names(_cc_sql_t *ctx, _cc_sql_result_t *res) {
    SQLCHAR names[256];
    SQLSMALLINT namelen, datatype, i;
    SQLRETURN res;
    int types;

    int32_t numcols = _sqlsvr_get_num_fields(res);
    for (i = 1; i <= numcols; i++) {
        res = SQLDescribeCol(res->hSTMT, i, names, sizeof(names),
            &namelen, &datatype, nullptr, nullptr, nullptr);
        if (res == SQL_ERROR) {
            return false;
        }
        printf("%d(%s)\n", i, names);
    }
}
#endif

_CC_API_PRIVATE(bool_t) _sqlsvr_next_result(_cc_sql_t *ctx, _cc_sql_result_t *result) {
    SQLRETURN res = SQL_SUCCESS;
    _cc_assert(ctx != nullptr && result != nullptr);

    res = SQLFetch(result->hSTMT);
    if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO)) {
        return false;
    }

    return true;
}

_CC_API_PRIVATE(bool_t) _sqlsvr_free_result(_cc_sql_t *ctx, _cc_sql_result_t *result) {
    _cc_assert(ctx != nullptr && result != nullptr);

    if (result->hSTMT != SQL_NULL_HSTMT) {
        SQLCloseCursor(result->hSTMT);
        SQLFreeStmt(result->hSTMT, SQL_CLOSE);
        SQLFreeStmt(result->hSTMT, SQL_DROP);
        SQLFreeHandle(SQL_HANDLE_STMT, result->hSTMT);
    }
    result->hSTMT = SQL_NULL_HSTMT;

    _cc_free(result);
    return true;
}

_CC_API_PRIVATE(bool_t) _sqlsvr_bind(_cc_sql_result_t *result, int32_t index, const void *value, size_t length, _sql_enum_field_types_t type) {
    _cc_logger_debug(_T("SQLServer _sqlsvr_bind: Not implemented yet"));
    return true;
}

/**/
_CC_API_PRIVATE(uint64_t) _sqlsvr_get_last_id(_cc_sql_t *ctx,_cc_sql_result_t *result) {
    _cc_assert(ctx != nullptr && ctx->hDBC != nullptr);
    _cc_logger_debug(_T("SQLServer get_last_id: Not implemented yet"));
    return -1;
}

_CC_API_PRIVATE(pvoid_t) _sqlsvr_get_stmt(_cc_sql_result_t *result) {
    _cc_assert(result != nullptr && result->hSTMT != nullptr);
    return result->hSTMT;
}

_CC_API_PRIVATE(int32_t) _sqlsvr_get_int(_cc_sql_result_t *result, int32_t index) {
    SQLLEN got = 0;
    SQLRETURN rc = 0;
    SQLINTEGER v;
    rc = SQLGetData(result->hSTMT, index + 1, SQL_INTEGER, (SQLPOINTER)&v, sizeof(SQLINTEGER), &got);
    if (sql_error(rc) || got == SQL_NULL_DATA) {
        return 0;
    }
    return v;
}

_CC_API_PRIVATE(int64_t) _sqlsvr_get_int64(_cc_sql_result_t *result, int32_t index) {
    SQLLEN got = 0;
    SQLRETURN rc = 0;
    SQLLEN v;
    rc = SQLGetData(result->hSTMT, index + 1, SQL_BIGINT, (SQLPOINTER)&v, sizeof(SQLLEN), &got);
    if (sql_error(rc) || got == SQL_NULL_DATA) {
        return 0;
    }
    return v;
}

_CC_API_PRIVATE(float64_t) _sqlsvr_get_float(_cc_sql_result_t *result, int32_t index) {
    SQLLEN got = 0;
    float64_t v;
    SQLRETURN rc = SQLGetData(result->hSTMT, index + 1, SQL_DOUBLE, (SQLPOINTER)&v, sizeof(float64_t), &got);
    if (sql_error(rc) || got == SQL_NULL_DATA) {
        return 0;
    }
    return v;
}

_CC_API_PRIVATE(size_t) _sqlsvr_get_string(_cc_sql_result_t *result, int32_t index, tchar_t *buffer, size_t length) {
    SQLLEN got = 0;
    SQLRETURN rc = SQLGetData(result->hSTMT, index + 1, SQL_CHAR, (SQLPOINTER)buffer, length, &got);
    if (sql_error(rc) || got == SQL_NULL_DATA) {
        return 0;
    }

    return got;
}

_CC_API_PRIVATE(size_t) _sqlsvr_get_blob(_cc_sql_result_t *result, int32_t index, byte_t **value) {
    _cc_assert(result->hSTMT != nullptr);
    _cc_logger_debug(_T("SQLServer _sqlsvr_get_blob: Not implemented yet"));
    return 0;
}

_CC_API_PRIVATE(bool_t) _sqlsvr_get_datetime(_cc_sql_result_t *result, int32_t index, struct tm* timeinfo) {
    /*
     SQLLEN got = 0;
    SQLRETURN rc = 0;
    _cc_assert(result != nullptr && result->hSTMT != nullptr);

    rc = SQLGetData(result->hSTMT, index + 1, SQL_C_TYPE_TIMESTAMP, (SQLPOINTER)dateString, _cc_countof(dateString), &got);

    if (sql_error(rc) || got == SQL_NULL_DATA) {
        return false;
    }

    _cc_strptime(dateString, fmt, tp);
    return true;*/
    return false;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_init_sqlsvr(_cc_sql_delegate_t *delegator) {
#define SET(x) delegator->x = _sqlsvr_##x
    if (_cc_unlikely(delegator == nullptr)) {
        return false;
    }

    /**/
    SET(connect);
    SET(disconnect);
    SET(execute);
    SET(auto_commit);
    SET(begin_transaction);
    SET(commit);
    SET(rollback);
    SET(next_result);
    SET(free_result);
    SET(fetch);
    SET(bind);

    SET(get_num_fields);
    SET(get_num_rows);
    SET(get_last_id);
    SET(get_stmt);
    SET(get_int);
    SET(get_int64);
    SET(get_float);
    SET(get_string);
    SET(get_blob);
    SET(get_datetime);

#undef SET

    return true;
}
