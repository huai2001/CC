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
#include <libcc/alloc.h>
#include <libcc/atomic.h>
#include <libcc/time.h>
#include <libcc/types.h>
#include <libcc/widgets/sql.h>
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
    bool_t step;
    SQLHSTMT hSTMT;
    _cc_buf_t buffer;
};

_CC_API_PRIVATE(int) is_odbc_error(SQLRETURN rc) {
    return (rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO) && (rc != SQL_NO_DATA);
}

/*
** Fails with error message from ODBC
** Inputs:
**   type: type of handle used in operation
**   handle: handle used in operation
*/

_CC_API_PRIVATE(void) _logger_fail_message_from_odbc(const SQLSMALLINT type, const SQLHANDLE handle, const tchar_t *action) {
    SQLCHAR sqlstate[SQL_SQLSTATE_SIZE + 1];
    SQLCHAR message[SQL_MAX_MESSAGE_LENGTH + 1];
    SQLINTEGER native_error;
    SQLSMALLINT length;
    
    SQLGetDiagRec(type, handle, 1, sqlstate, &native_error,  message, sizeof(message), &length);
    _cc_logger_error(_T("Error in : %s SQLSTATE: %s Message: %s"), action, sqlstate, message);
}

_CC_API_PRIVATE(bool_t) _init_sqlsvr(void) {
    int request = 0;

    if (_cc_unlikely(hEnv != SQL_NULL_HENV)) {
        return true;
    }

    /*Allocate the ODBC environment and save handle.*/
    request = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    if ((request != SQL_SUCCESS_WITH_INFO) && (request != SQL_SUCCESS)) {  
        _cc_logger(_CC_LOG_LEVEL_ERROR_, _T("SQLAllocHandle(Env) Failed"));
        return false;
    }

    /*Notify ODBC that this is an ODBC 3.0 app.*/
    request = SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER *)SQL_OV_ODBC3, SQL_IS_INTEGER);
    if ((request != SQL_SUCCESS_WITH_INFO) && (request != SQL_SUCCESS)) {  
        _cc_logger(_CC_LOG_LEVEL_ERROR_, _T("SQLSetEnvAttr(ODBC version) Failed"));
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

    if (is_odbc_error(request)) {
        _cc_free(ctx);
        return nullptr;
    }

    SQLSetConnectAttr(ctx->hDBC, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

    request = SQLDriverConnect(ctx->hDBC, nullptr, (SQLTCHAR *)sql_connection_string,
                               (SQLSMALLINT)_tcslen(sql_connection_string), OutConnStr,
                               (SQLSMALLINT)_cc_countof(OutConnStr), &OutConnStrLen, SQL_DRIVER_NOPROMPT);

    if (is_odbc_error(request)) {
        _logger_fail_message_from_odbc(SQL_HANDLE_DBC,ctx->hDBC, _T("SQLDriverConnect"));
        SQLFreeHandle(SQL_HANDLE_DESC, ctx->hDBC);
        ctx->hDBC = SQL_NULL_HDBC;
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

_CC_API_PRIVATE(bool_t) SQLExecute_ex(SQLHSTMT hSTMT) {
    int rc = SQLExecute(hSTMT);

    if (rc == SQL_NEED_DATA) {
        char buf[_CC_4K_BUFFER_SIZE_];
        int fp;
        SQLLEN nbytes;
        do {
            rc = SQLParamData(hSTMT, (SQLPOINTER *)&fp);
            if (rc == SQL_NEED_DATA) {
                while ((nbytes = (SQLLEN)read(fp, &buf, _CC_4K_BUFFER_SIZE_)) > 0) {
                    SQLPutData(hSTMT, (void *)&buf, nbytes);
                }
            }
        } while (rc == SQL_NEED_DATA);
    }

    if (is_odbc_error(rc)) {
        _logger_fail_message_from_odbc(SQL_HANDLE_STMT, hSTMT, _T("SQLExecute"));
        return false;
    }

    return true;
}

_CC_API_PRIVATE(bool_t) _sqlsvr_execute(_cc_sql_t *ctx, const _cc_String_t *sql, _cc_sql_result_t **result) {
    SQLHSTMT hSTMT = SQL_NULL_HSTMT;
    _cc_assert(ctx != nullptr && ctx->hDBC != SQL_NULL_HSTMT);

    if (is_odbc_error(SQLAllocStmt(ctx->hDBC, &hSTMT))) {
        _logger_fail_message_from_odbc(SQL_HANDLE_DBC, ctx->hDBC, _T("SQLAllocStmt"));
        return false;
    }

    if (_cc_unlikely(hSTMT == SQL_NULL_HSTMT)) {
        return false;
    }

    if (is_odbc_error(SQLPrepare(hSTMT, (SQLTCHAR *)sql->data, SQL_NTS))) {
        _logger_fail_message_from_odbc(SQL_HANDLE_STMT, hSTMT, _T("SQLPrepare"));
        SQLCloseCursor(hSTMT);
        SQLFreeStmt(hSTMT, SQL_CLOSE);
        SQLFreeStmt(hSTMT, SQL_DROP);
        SQLFreeHandle(SQL_HANDLE_STMT, hSTMT);
        return false;
    }

    if (result == nullptr) {
        bool_t rc = SQLExecute_ex(hSTMT);
        SQLCloseCursor(hSTMT);
        SQLFreeStmt(hSTMT, SQL_CLOSE);
        SQLFreeStmt(hSTMT, SQL_DROP);
        SQLFreeHandle(SQL_HANDLE_STMT, hSTMT);
        return rc;
    } else {
        *result = (_cc_sql_result_t *)_cc_malloc(sizeof(_cc_sql_result_t));
        (*result)->hSTMT = hSTMT;
        (*result)->buffer.bytes = nullptr;
        (*result)->buffer.limit = 0;
        (*result)->buffer.length = 0;
    }
    return true;
}

_CC_API_PRIVATE(bool_t) _sqlsvr_reset(_cc_sql_t *ctx, _cc_sql_result_t *result) {
    _cc_assert(result != nullptr && result->hSTMT != SQL_NULL_HSTMT);
    if (is_odbc_error(SQLFreeStmt(result->hSTMT, SQL_RESET_PARAMS))) {
        _logger_fail_message_from_odbc(SQL_HANDLE_STMT, result->hSTMT, _T("SQLFreeStmt"));
        return false;
    }
    return true;
}

_CC_API_PRIVATE(bool_t) _sqlsvr_step(_cc_sql_t *ctx, _cc_sql_result_t *result) {
    _cc_assert(result != nullptr && result->hSTMT != SQL_NULL_HSTMT);
    result->step = true;
    return SQLExecute_ex(result->hSTMT);
}

_CC_API_PRIVATE(bool_t) _sqlsvr_auto_commit(_cc_sql_t *ctx, bool_t is_auto_commit) {
    SQLRETURN rc = 0;
    _cc_assert(ctx != nullptr && ctx->hDBC != SQL_NULL_HDBC);

    if (is_auto_commit) {
        rc = SQLSetConnectAttr(ctx->hDBC, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
    } else {
        rc = SQLSetConnectAttr(ctx->hDBC, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0);
    }

    if (is_odbc_error(rc)) {
        _logger_fail_message_from_odbc(SQL_HANDLE_DBC, ctx->hDBC, _T("SQLSetConnectAttr"));
        return false;
    }
    return true;
}

_CC_API_PRIVATE(bool_t) _sqlsvr_begin_transaction(_cc_sql_t *ctx) {
    _cc_assert(ctx != nullptr && ctx->hDBC != SQL_NULL_HDBC);
    if (!ctx->auto_commit) {
        return true;
    }

    return _sqlsvr_auto_commit(ctx, false);
}

_CC_API_PRIVATE(bool_t) _sqlsvr_commit(_cc_sql_t *ctx) {
    _cc_assert(ctx != nullptr && ctx->hDBC != SQL_NULL_HDBC);
    if (is_odbc_error(SQLEndTran(SQL_HANDLE_DBC, ctx->hDBC, SQL_COMMIT))) {
        _logger_fail_message_from_odbc(SQL_HANDLE_DBC, ctx->hDBC, _T("SQLEndTran(SQL_COMMIT)"));
        return false;
    }
    return true;
}

_CC_API_PRIVATE(bool_t) _sqlsvr_rollback(_cc_sql_t *ctx) {
    _cc_assert(ctx != nullptr && ctx->hDBC != SQL_NULL_HDBC);
    if (is_odbc_error(SQLEndTran(SQL_HANDLE_DBC, ctx->hDBC, SQL_ROLLBACK))) {
        _logger_fail_message_from_odbc(SQL_HANDLE_DBC, ctx->hDBC, _T("SQLEndTran(SQL_ROLLBACK)"));
        return false;
    }
    return true;
}

_CC_API_PRIVATE(uint64_t) _sqlsvr_get_num_rows(_cc_sql_result_t *result) {
    SQLLEN numrows;
    _cc_assert(result != nullptr && result->hSTMT != SQL_NULL_HSTMT);
    /* determine the number of result rows */
    if (is_odbc_error(SQLRowCount(result->hSTMT, (SQLLEN *)&numrows))) {
        _logger_fail_message_from_odbc(SQL_HANDLE_STMT, result->hSTMT, _T("SQLRowCount"));
        return 0;
    }
    return (uint64_t)numrows;
}

_CC_API_PRIVATE(bool_t) _sqlsvr_fetch(_cc_sql_result_t *result) {
    SQLRETURN res;
    _cc_assert(result != nullptr && result->hSTMT != SQL_NULL_HSTMT);
    if (!result->step) {
        result->step = true;
        if (!SQLExecute_ex(result->hSTMT)) {
            return false;
        }
    }

    res = SQLFetch(result->hSTMT);
    if (res == SQL_NO_DATA) {
        return false;
    }

    if (res != SQL_SUCCESS) {
        _logger_fail_message_from_odbc(SQL_HANDLE_STMT, result->hSTMT, _T("SQLFetch"));
        return false;
    }

    return true;
}

_CC_API_PRIVATE(int32_t) _sqlsvr_get_num_fields(_cc_sql_result_t *result) {
    SQLSMALLINT numcols;
    _cc_assert(result != nullptr && result->hSTMT != SQL_NULL_HSTMT);
    /* determine the number of result columns */
    if (is_odbc_error(SQLNumResultCols(result->hSTMT, &numcols))) {
        _logger_fail_message_from_odbc(SQL_HANDLE_STMT, result->hSTMT, _T("SQLNumResultCols"));
        return false;
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
    _cc_assert(result != nullptr && result->hSTMT != SQL_NULL_HSTMT);
    if (is_odbc_error(SQLFetch(result->hSTMT))) {
        _logger_fail_message_from_odbc(SQL_HANDLE_STMT, result->hSTMT, _T("SQLFetch"));
        return false;
    }
    return true;
}

_CC_API_PRIVATE(bool_t) _sqlsvr_free_result(_cc_sql_t *ctx, _cc_sql_result_t *result) {
    _cc_assert(result != nullptr && result->hSTMT != SQL_NULL_HSTMT);

    if (result->hSTMT != SQL_NULL_HSTMT) {
        SQLCloseCursor(result->hSTMT);
        SQLFreeStmt(result->hSTMT, SQL_CLOSE);
        SQLFreeStmt(result->hSTMT, SQL_DROP);
        SQLFreeHandle(SQL_HANDLE_STMT, result->hSTMT);
    }
    result->hSTMT = SQL_NULL_HSTMT;
    if (result->buffer.limit > 0) {
        _cc_free_buf(&result->buffer);
    }
    _cc_free(result);
    return true;
}

_CC_API_PRIVATE(bool_t) _sqlsvr_bind(_cc_sql_result_t *result, int32_t index, const void *value, size_t length, _sql_enum_field_types_t type) {
    SQLRETURN res;
    SQLLEN length_ind = 0;
    _cc_assert(result != nullptr);
    /*List the serial numbers (starting from 1)*/
    index++;
    switch(type) {
        case _CC_SQL_TYPE_INT8_:
        case _CC_SQL_TYPE_UINT8_:
            res = SQLBindParameter(result->hSTMT, index, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, sizeof(int8_t), 0, (SQLPOINTER)value, length, &length_ind);
            break;
        case _CC_SQL_TYPE_INT16_:
        case _CC_SQL_TYPE_UINT16_:
            res = SQLBindParameter(result->hSTMT, index, SQL_PARAM_INPUT, SQL_C_SHORT, SQL_SMALLINT, sizeof(int16_t), 0, (SQLPOINTER)value, length, &length_ind);
            break;
        case _CC_SQL_TYPE_INT32_:
        case _CC_SQL_TYPE_UINT32_:
        case _CC_SQL_TYPE_TIMESTAMP_:
            res = SQLBindParameter(result->hSTMT, index, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, sizeof(int32_t), 0, (SQLPOINTER)value, length, &length_ind);
            break;
        case _CC_SQL_TYPE_INT64_:
            res = SQLBindParameter(result->hSTMT, index, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, sizeof(int64_t), 0, (SQLPOINTER)value, length, &length_ind);
            break;
        case _CC_SQL_TYPE_FLOAT_:
        case _CC_SQL_TYPE_DOUBLE_:
            res = SQLBindParameter(result->hSTMT, index, SQL_PARAM_INPUT, SQL_C_DOUBLE, SQL_DOUBLE, sizeof(double), 0, (SQLPOINTER)value, length, &length_ind);
            break;
        case _CC_SQL_TYPE_STRING_:
        case _CC_SQL_TYPE_JSON_:{
            /*Question? Remove the SQLLEN declaration as a local variable, the string of the database field is null.*/
           SQLLEN str_ind = SQL_NTS;
            if (length == -1) {
                length = 0;
            }
            res = SQLBindParameter(result->hSTMT, index, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, (SQLULEN)length, 0, (SQLPOINTER*)value, (SQLULEN)length, &str_ind);
        }
            break;
        case _CC_SQL_TYPE_DATETIME_: {
            struct tm *tm_v = (struct tm *)value;
            SQL_TIMESTAMP_STRUCT ts = {
                                    tm_v->tm_year + 1900, 
                                    tm_v->tm_mon + 1, 
                                    tm_v->tm_mday, 
                                    tm_v->tm_hour, 
                                    tm_v->tm_min, 
                                    tm_v->tm_sec,
                                    0
                                    };
            res = SQLBindParameter(result->hSTMT, index, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP, 23, 3, &ts, sizeof(SQL_TIMESTAMP_STRUCT),&length_ind);
            break;
        }
        case _CC_SQL_TYPE_BLOB_: {
            SQLLEN blod_ind = length;
            res = SQLBindParameter(result->hSTMT, index, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_LONGVARBINARY, (SQLULEN)length, 0, (SQLPOINTER)value, (SQLULEN)length, &blod_ind);
        }
            break;
        case _CC_SQL_TYPE_NULL_: {
            SQLLEN null_ind = SQL_NULL_DATA;
            res = SQLBindParameter(result->hSTMT, index, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 0, 0, nullptr, 0, &null_ind);
        }
            break;
        default:
            return false;
    }
    if (is_odbc_error(res)) {
        _logger_fail_message_from_odbc(SQL_HANDLE_STMT, result->hSTMT, _T("SQLBindParameter"));
        return false;
    }
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
    if (is_odbc_error(rc) || got == SQL_NULL_DATA) {
        return 0;
    }
    return v;
}

_CC_API_PRIVATE(int64_t) _sqlsvr_get_int64(_cc_sql_result_t *result, int32_t index) {
    SQLLEN got = 0;
    SQLRETURN rc = 0;
    SQLLEN v;
    rc = SQLGetData(result->hSTMT, index + 1, SQL_BIGINT, (SQLPOINTER)&v, sizeof(SQLLEN), &got);
    if (is_odbc_error(rc) || got == SQL_NULL_DATA) {
        return 0;
    }
    return v;
}

_CC_API_PRIVATE(float64_t) _sqlsvr_get_float(_cc_sql_result_t *result, int32_t index) {
    SQLLEN got = 0;
    float64_t v;
    SQLRETURN rc = SQLGetData(result->hSTMT, index + 1, SQL_DOUBLE, (SQLPOINTER)&v, sizeof(float64_t), &got);
    if (is_odbc_error(rc) || got == SQL_NULL_DATA) {
        return 0;
    }
    return v;
}

_CC_API_PRIVATE(size_t) _sqlsvr_get_string(_cc_sql_result_t *result, int32_t index, tchar_t *buffer, size_t length) {
    SQLLEN got = 0;
    SQLRETURN rc = SQLGetData(result->hSTMT, index + 1, SQL_CHAR, (SQLPOINTER)buffer, length, &got);
    if (is_odbc_error(rc) || got == SQL_NULL_DATA) {
        return 0;
    }

    return got;
}

_CC_API_PRIVATE(size_t) _sqlsvr_get_blob(_cc_sql_result_t *result, int32_t index, byte_t **value) {
    SQLLEN got = 0;
    SQLRETURN rc;
    if (result->buffer.limit == 0) {
        _cc_alloc_buf(&result->buffer, _CC_1K_BUFFER_SIZE_);
    }

    do {
        SQLLEN chunSize = (SQLLEN)(result->buffer.limit - result->buffer.length);
        if (chunSize < _CC_1K_BUFFER_SIZE_) {
            _cc_buf_expand(&result->buffer,_CC_1K_BUFFER_SIZE_);
        }

        rc = SQLGetData(result->hSTMT, index + 1, SQL_C_BINARY, (SQLPOINTER)(result->buffer.bytes + result->buffer.length), chunSize, &got);
        if (got != SQL_NULL_DATA) {
            result->buffer.length += got;
        }

    } while (rc == SQL_SUCCESS_WITH_INFO);
    if (is_odbc_error(rc)) {
        return 0;
    }

    *value = result->buffer.bytes;
    return result->buffer.length;
}

_CC_API_PRIVATE(bool_t) _sqlsvr_get_datetime(_cc_sql_result_t *result, int32_t index, struct tm* timeinfo) {
    TIMESTAMP_STRUCT ts;
    SQLLEN got;
    if (is_odbc_error(SQLGetData(result->hSTMT, index + 1, SQL_C_TYPE_TIMESTAMP, &ts, sizeof(ts), &got))) {
        return false;
    }

    timeinfo->tm_sec = ts.second;
    timeinfo->tm_min = ts.minute;
    timeinfo->tm_hour = ts.hour;
    timeinfo->tm_isdst = -1;

    if (ts.year >= 1900 && ts.month > 0) {
        timeinfo->tm_mday = ts.day;
        timeinfo->tm_mon = ts.month - 1;
        timeinfo->tm_year = ts.year - 1900;
    }

    _cc_civil_to_days(ts.year,ts.month,ts.day,&timeinfo->tm_wday,&timeinfo->tm_yday);

    return true;
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
    SET(reset);
    SET(step);
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
