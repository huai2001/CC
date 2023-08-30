/*
 * Copyright .Qiu<huai2011@163.com>. and other libCC contributors.
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
#include <cc/alloc.h>
#include <cc/atomic.h>
#include <cc/db/sql.h>
#include <cc/time.h>
#include <cc/types.h>
#include <cc/math.h>



#if _CC_USE_SYSTEM_SQLITE3_LIB_
#include <sqlite3.h>
#else
#include <sqlite3/sqlite3.h>
#endif

/*
////////////////////////////////////////////////////////////////////////
//
// TCHAR based sqlite3 function names for Unicode/MCBS builds.
//
*/
#if defined(_UNICODE) || defined(UNICODE)
/*#pragma message("Unicode Selected")*/
#define _sqlite3_aggregate_context sqlite3_aggregate_context
#define _sqlite3_aggregate_count sqlite3_aggregate_count
#define _sqlite3_bind_blob sqlite3_bind_blob
#define _sqlite3_bind_double sqlite3_bind_double
#define _sqlite3_bind_int sqlite3_bind_int
#define _sqlite3_bind_int64 sqlite3_bind_int64
#define _sqlite3_bind_null sqlite3_bind_null
#define _sqlite3_bind_fieldeter_count sqlite3_bind_fieldeter_count
#define _sqlite3_bind_fieldeter_index sqlite3_bind_fieldeter_index
#define _sqlite3_bind_fieldeter_name sqlite3_bind_fieldeter_name
#define _sqlite3_bind_text sqlite3_bind_text16
#define _sqlite3_bind_text16 sqlite3_bind_text16
#define _sqlite3_busy_handler sqlite3_busy_handler
#define _sqlite3_busy_timeout sqlite3_busy_timeout
#define _sqlite3_changes sqlite3_changes
#define _sqlite3_close sqlite3_close
#define _sqlite3_collation_needed sqlite3_collation_needed16
#define _sqlite3_collation_needed16 sqlite3_collation_needed16
#define _sqlite3_column_blob sqlite3_column_blob
#define _sqlite3_column_bytes sqlite3_column_bytes16
#define _sqlite3_column_bytes16 sqlite3_column_bytes16
#define _sqlite3_column_count sqlite3_column_count
#define _sqlite3_column_decltype sqlite3_column_decltype16
#define _sqlite3_column_decltype16 sqlite3_column_decltype16
#define _sqlite3_column_double sqlite3_column_double
#define _sqlite3_column_int sqlite3_column_int
#define _sqlite3_column_int64 sqlite3_column_int64
#define _sqlite3_column_name sqlite3_column_name16
#define _sqlite3_column_name16 sqlite3_column_name16
#define _sqlite3_column_text sqlite3_column_text16
#define _sqlite3_column_text16 sqlite3_column_text16
#define _sqlite3_column_type sqlite3_column_type
#define _sqlite3_commit_hook sqlite3_commit_hook
#define _sqlite3_complete sqlite3_complete16
#define _sqlite3_complete16 sqlite3_complete16
#define _sqlite3_create_collation sqlite3_create_collation16
#define _sqlite3_create_collation16 sqlite3_create_collation16
#define _sqlite3_create_function sqlite3_create_function16
#define _sqlite3_create_function16 sqlite3_create_function16
#define _sqlite3_data_count sqlite3_data_count
#define _sqlite3_errcode sqlite3_errcode
#define _sqlite3_errmsg sqlite3_errmsg16
#define _sqlite3_errmsg16 sqlite3_errmsg16
#define _sqlite3_exec sqlite3_exec
#define _sqlite3_finalize sqlite3_finalize
#define _sqlite3_free sqlite3_free
#define _sqlite3_free_table sqlite3_free_table
#define _sqlite3_get_table sqlite3_get_table
#define _sqlite3_interrupt sqlite3_interrupt
#define _sqlite3_last_insert_rowid sqlite3_last_insert_rowid
#define _sqlite3_libversion sqlite3_libversion
#define _sqlite3_mprintf sqlite3_mprintf
#define _sqlite3_open sqlite3_open16
#define _sqlite3_open16 sqlite3_open16
#define _sqlite3_prepare sqlite3_prepare16_v2
#define _sqlite3_prepare16 sqlite3_prepare16_v2
#define _sqlite3_progress_handler sqlite3_progress_handler
#define _sqlite3_reset sqlite3_reset
#define _sqlite3_result_blob sqlite3_result_blob
#define _sqlite3_result_double sqlite3_result_double
#define _sqlite3_result_error sqlite3_result_error16
#define _sqlite3_result_error16 sqlite3_result_error16
#define _sqlite3_result_int sqlite3_result_int
#define _sqlite3_result_int64 sqlite3_result_int64
#define _sqlite3_result_null sqlite3_result_null
#define _sqlite3_result_text sqlite3_result_text16
#define _sqlite3_result_text16 sqlite3_result_text16
#define _sqlite3_result_text16be sqlite3_result_text16be
#define _sqlite3_result_text16le sqlite3_result_text16le
#define _sqlite3_result_fieldValue sqlite3_result_fieldValue
#define _sqlite3_set_authorizer sqlite3_set_authorizer
#define _sqlite3_step sqlite3_step
#define _sqlite3_total_changes sqlite3_total_changes
#define _sqlite3_trace sqlite3_trace
#define _sqlite3_user_data sqlite3_user_data
#define _sqlite3_fieldValue_blob sqlite3_fieldValue_blob
#define _sqlite3_fieldValue_bytes sqlite3_fieldValue_bytes16
#define _sqlite3_fieldValue_bytes16 sqlite3_fieldValue_bytes16
#define _sqlite3_fieldValue_double sqlite3_fieldValue_double
#define _sqlite3_fieldValue_int sqlite3_fieldValue_int
#define _sqlite3_fieldValue_int64 sqlite3_fieldValue_int64
#define _sqlite3_fieldValue_text sqlite3_fieldValue_text16
#define _sqlite3_fieldValue_text16 sqlite3_fieldValue_text16
#define _sqlite3_fieldValue_text16be sqlite3_fieldValue_text16be
#define _sqlite3_fieldValue_text16le sqlite3_fieldValue_text16le
#define _sqlite3_fieldValue_type sqlite3_fieldValue_type
#define _sqlite3_vmprintf sqlite3_vmprintf
#else
/*#pragma message("MCBS Selected")*/
#define _sqlite3_aggregate_context sqlite3_aggregate_context
#define _sqlite3_aggregate_count sqlite3_aggregate_count
#define _sqlite3_bind_blob sqlite3_bind_blob
#define _sqlite3_bind_double sqlite3_bind_double
#define _sqlite3_bind_int sqlite3_bind_int
#define _sqlite3_bind_int64 sqlite3_bind_int64
#define _sqlite3_bind_null sqlite3_bind_null
#define _sqlite3_bind_fieldeter_count sqlite3_bind_fieldeter_count
#define _sqlite3_bind_fieldeter_index sqlite3_bind_fieldeter_index
#define _sqlite3_bind_fieldeter_name sqlite3_bind_fieldeter_name
#define _sqlite3_bind_text sqlite3_bind_text
#define _sqlite3_bind_text16 sqlite3_bind_text16
#define _sqlite3_busy_handler sqlite3_busy_handler
#define _sqlite3_busy_timeout sqlite3_busy_timeout
#define _sqlite3_changes sqlite3_changes
#define _sqlite3_close sqlite3_close
#define _sqlite3_collation_needed sqlite3_collation_needed
#define _sqlite3_collation_needed16 sqlite3_collation_needed16
#define _sqlite3_column_blob sqlite3_column_blob
#define _sqlite3_column_bytes sqlite3_column_bytes
#define _sqlite3_column_bytes16 sqlite3_column_bytes16
#define _sqlite3_column_count sqlite3_column_count
#define _sqlite3_column_decltype sqlite3_column_decltype
#define _sqlite3_column_decltype16 sqlite3_column_decltype16
#define _sqlite3_column_double sqlite3_column_double
#define _sqlite3_column_int sqlite3_column_int
#define _sqlite3_column_int64 sqlite3_column_int64
#define _sqlite3_column_name sqlite3_column_name
#define _sqlite3_column_name16 sqlite3_column_name16
#define _sqlite3_column_text sqlite3_column_text
#define _sqlite3_column_text16 sqlite3_column_text16
#define _sqlite3_column_type sqlite3_column_type
#define _sqlite3_commit_hook sqlite3_commit_hook
#define _sqlite3_complete sqlite3_complete
#define _sqlite3_complete16 sqlite3_complete16
#define _sqlite3_create_collation sqlite3_create_collation
#define _sqlite3_create_collation16 sqlite3_create_collation16
#define _sqlite3_create_function sqlite3_create_function
#define _sqlite3_create_function16 sqlite3_create_function16
#define _sqlite3_data_count sqlite3_data_count
#define _sqlite3_errcode sqlite3_errcode
#define _sqlite3_errmsg sqlite3_errmsg
#define _sqlite3_errmsg16 sqlite3_errmsg16
#define _sqlite3_exec sqlite3_exec
#define _sqlite3_finalize sqlite3_finalize
#define _sqlite3_free sqlite3_free
#define _sqlite3_free_table sqlite3_free_table
#define _sqlite3_get_table sqlite3_get_table
#define _sqlite3_interrupt sqlite3_interrupt
#define _sqlite3_last_insert_rowid sqlite3_last_insert_rowid
#define _sqlite3_libversion sqlite3_libversion
#define _sqlite3_mprintf sqlite3_mprintf
#define _sqlite3_open sqlite3_open
#define _sqlite3_open16 sqlite3_open16
#define _sqlite3_prepare sqlite3_prepare_v2
#define _sqlite3_prepare16 sqlite3_prepare16_v2
#define _sqlite3_progress_handler sqlite3_progress_handler
#define _sqlite3_reset sqlite3_reset
#define _sqlite3_result_blob sqlite3_result_blob
#define _sqlite3_result_double sqlite3_result_double
#define _sqlite3_result_error sqlite3_result_error
#define _sqlite3_result_error16 sqlite3_result_error16
#define _sqlite3_result_int sqlite3_result_int
#define _sqlite3_result_int64 sqlite3_result_int64
#define _sqlite3_result_null sqlite3_result_null
#define _sqlite3_result_text sqlite3_result_text
#define _sqlite3_result_text16 sqlite3_result_text16
#define _sqlite3_result_text16be sqlite3_result_text16be
#define _sqlite3_result_text16le sqlite3_result_text16le
#define _sqlite3_result_fieldValue sqlite3_result_fieldValue
#define _sqlite3_set_authorizer sqlite3_set_authorizer
#define _sqlite3_step sqlite3_step
#define _sqlite3_total_changes sqlite3_total_changes
#define _sqlite3_trace sqlite3_trace
#define _sqlite3_user_data sqlite3_user_data
#define _sqlite3_fieldValue_blob sqlite3_fieldValue_blob
#define _sqlite3_fieldValue_bytes sqlite3_fieldValue_bytes
#define _sqlite3_fieldValue_bytes16 sqlite3_fieldValue_bytes16
#define _sqlite3_fieldValue_double sqlite3_fieldValue_double
#define _sqlite3_fieldValue_int sqlite3_fieldValue_int
#define _sqlite3_fieldValue_int64 sqlite3_fieldValue_int64
#define _sqlite3_fieldValue_text sqlite3_fieldValue_text
#define _sqlite3_fieldValue_text16 sqlite3_fieldValue_text16
#define _sqlite3_fieldValue_text16be sqlite3_fieldValue_text16be
#define _sqlite3_fieldValue_text16le sqlite3_fieldValue_text16le
#define _sqlite3_fieldValue_type sqlite3_fieldValue_type
#define _sqlite3_vmprintf sqlite3_vmprintf
#endif
/*//////////////////////////////////////////////////////////////////////////////////*/

struct _cc_sql {
    sqlite3 *sql;
    bool_t transaction;
};

struct _cc_sql_result {
    int step_status;
    sqlite3_stmt *stmt;
};
/*
static const tchar_t* code_as_string(const int32_t err_code) {
    switch (err_code) {
    case SQLITE_OK          : return _T("SQLITE_OK");
    case SQLITE_ERROR       : return _T("SQLITE_ERROR");
    case SQLITE_INTERNAL    : return _T("SQLITE_INTERNAL");
    case SQLITE_PERM        : return _T("SQLITE_PERM");
    case SQLITE_ABORT       : return _T("SQLITE_ABORT");
    case SQLITE_BUSY        : return _T("SQLITE_BUSY");
    case SQLITE_LOCKED      : return _T("SQLITE_LOCKED");
    case SQLITE_NOMEM       : return _T("SQLITE_NOMEM");
    case SQLITE_READONLY    : return _T("SQLITE_READONLY");
    case SQLITE_INTERRUPT   : return _T("SQLITE_INTERRUPT");
    case SQLITE_IOERR       : return _T("SQLITE_IOERR");
    case SQLITE_CORRUPT     : return _T("SQLITE_CORRUPT");
    case SQLITE_NOTFOUND    : return _T("SQLITE_NOTFOUND");
    case SQLITE_FULL        : return _T("SQLITE_FULL");
    case SQLITE_CANTOPEN    : return _T("SQLITE_CANTOPEN");
    case SQLITE_PROTOCOL    : return _T("SQLITE_PROTOCOL");
    case SQLITE_EMPTY       : return _T("SQLITE_EMPTY");
    case SQLITE_SCHEMA      : return _T("SQLITE_SCHEMA");
    case SQLITE_TOOBIG      : return _T("SQLITE_TOOBIG");
    case SQLITE_CONSTRAINT  : return _T("SQLITE_CONSTRAINT");
    case SQLITE_MISMATCH    : return _T("SQLITE_MISMATCH");
    case SQLITE_MISUSE      : return _T("SQLITE_MISUSE");
    case SQLITE_NOLFS       : return _T("SQLITE_NOLFS");
    case SQLITE_AUTH        : return _T("SQLITE_AUTH");
    case SQLITE_FORMAT      : return _T("SQLITE_FORMAT");
    case SQLITE_RANGE       : return _T("SQLITE_RANGE");
    case SQLITE_ROW         : return _T("SQLITE_ROW");
    case SQLITE_DONE        : return _T("SQLITE_DONE");
    default: return _T("UNKNOWN_ERROR");
    }
}
*/

static _cc_sql_t *_sqlite_connect(const tchar_t *sql_connection_string) {
    sqlite3 *sql = NULL;
    _cc_sql_t *ctx = NULL;
    _cc_url_t params;

    if (!_cc_parse_url(&params, sql_connection_string)) {
        return NULL;
    }

    if (_sqlite3_open(params.path + 1, &sql) != SQLITE_OK) {
        _cc_logger_error(_T("Can't open database: %s, %s"), params.path + 1, _sqlite3_errmsg(sql));
        _cc_free_url(&params);
        return NULL;
    }

    _cc_free_url(&params);
    ctx = (_cc_sql_t *)_cc_malloc(sizeof(_cc_sql_t));
    ctx->sql = sql;
    ctx->transaction = false;

//    sqlite3_exec(ctx->sql, "PRAGMA synchronous = OFF", 0, 0, 0);
//    sqlite3_exec(ctx->sql, "PRAGMA cache_size = 8000", 0, 0, 0);
//    sqlite3_exec(ctx->sql, "PRAGMA count_changes = 1", 0, 0, 0);
//    sqlite3_exec(ctx->sql, "PRAGMA case_sensitive_like = 1", 0, 0, 0);

    sqlite3_exec(ctx->sql, "PRAGMA secure_delete = ON", 0, 0, 0);
    sqlite3_exec(ctx->sql, "PRAGMA temp_store = MEMORY", 0, 0, 0);
    sqlite3_exec(ctx->sql, "PRAGMA journal_mode = WAL", 0, 0, 0);
    sqlite3_exec(ctx->sql, "PRAGMA journal_size_limit = 10485760", 0, 0, 0);

    return ctx;
}

static bool_t _sqlite_disconnect(_cc_sql_t *ctx) {
    _cc_assert(ctx != NULL);

    if (ctx->sql) {
        int res = _sqlite3_close(ctx->sql);
        if (res != SQLITE_OK) {
            _cc_logger_error(_T("_sqlite3_close: %s"), _sqlite3_errmsg(ctx->sql));
            return false;
        }
        _cc_free(ctx);
    }
    return true;
}

static bool_t _sqlite_prepare(_cc_sql_t *ctx, const tchar_t *sql_string, _cc_sql_result_t **result) {
    int res = SQLITE_OK;
    sqlite3_stmt *stmt = NULL;
    const tchar_t *tail;

    _cc_assert(ctx != NULL && ctx->sql != NULL);

    while (res == SQLITE_OK && *sql_string) {
        res = _sqlite3_prepare(ctx->sql, sql_string, -1, &stmt, &tail);
        if (res != SQLITE_OK) {
            _cc_logger_error(_T("_sqlite3_prepare: %s"), _sqlite3_errmsg(ctx->sql));
            if (stmt) {
                _sqlite3_finalize(stmt);
            }
            return false;
        }

        if (!stmt) {
            /* this happens for a comment or white-space */
            sql_string = tail;
            continue;
        }

        break;
    }

    if (result) {
        *result = (_cc_sql_result_t*)_cc_malloc(sizeof(_cc_sql_result_t));
        (*result)->stmt = stmt;
        (*result)->step_status = SQLITE_OK;
        return true;
    }
    return false;
}

static bool_t _sqlite_reset(_cc_sql_t *ctx, _cc_sql_result_t *result) {
    int res = _sqlite3_reset(result->stmt);
    if (SQLITE_OK != res) {
        _cc_logger_error(_T("_sqlite3_reset: %s"), _sqlite3_errmsg(ctx->sql));
        return false;
    }
    result->step_status = res;
    return true;
}

static bool_t _sqlite_step(_cc_sql_t *ctx, _cc_sql_result_t *result) {
    int res = _sqlite3_step(result->stmt);
    if ((res != SQLITE_OK) && (res != SQLITE_DONE) && (res != SQLITE_ROW)) {
        _cc_logger_error(_T("_sqlite3_step: %s"), _sqlite3_errmsg(ctx->sql));
        return false;
    }
    result->step_status = res;
    return true;
}

static bool_t _sqlite_execute(_cc_sql_t *ctx, const tchar_t *sql_string, _cc_sql_result_t **result) {
    int res = SQLITE_OK;
    sqlite3_stmt *stmt = NULL;
    const tchar_t *tail;

    _cc_assert(ctx != NULL && ctx->sql != NULL);

    while (res == SQLITE_OK && *sql_string) {
        res = _sqlite3_prepare(ctx->sql, sql_string, -1, &stmt, &tail);
        if (res != SQLITE_OK) {
            _cc_logger_error(_T("_sqlite3_prepare: %s"), _sqlite3_errmsg(ctx->sql));
            if (stmt) {
                _sqlite3_finalize(stmt);
            }
            return false;
        }

        if (!stmt) {
            /* this happens for a comment or white-space */
            sql_string = tail;
            continue;
        }

        break;
    }

    if (result) {
        *result = (_cc_sql_result_t*)_cc_malloc(sizeof(_cc_sql_result_t));
        (*result)->stmt = stmt;
        (*result)->step_status = SQLITE_OK;
        return true;
    }

    res = _sqlite3_step(stmt);
    if ((res != SQLITE_OK) && (res != SQLITE_DONE) && (res != SQLITE_ROW)) {
        _cc_logger_error(_T("_sqlite3_step: %s"), _sqlite3_errmsg(ctx->sql));
        res = SQLITE_ERROR;
    }

    _sqlite3_finalize(stmt);
    return (res != SQLITE_ERROR);
}

static bool_t _sqlite_auto_commit(_cc_sql_t *ctx, bool_t is_auto_commit) {
    _cc_assert(ctx != NULL);

    if (is_auto_commit) {
        /* undo active transaction - ignore errors */
        (void)sqlite3_exec(ctx->sql, "ROLLBACK", NULL, NULL, NULL);
        ctx->transaction = false;
    } else {
        int res = 0;
        char_t *errmsg = NULL;

        res = _sqlite3_exec(ctx->sql, "BEGIN", NULL, NULL, &errmsg);

        if (res != SQLITE_OK) {
            _cc_logger_error(_T("_sqlite_auto_commit %s"), errmsg);
            _sqlite3_free(errmsg);
            return false;
        }
        ctx->transaction = true;
    }

    return true;
}

static bool_t _sqlite_begin_transaction(_cc_sql_t *ctx) {
    int res = 0;
    char_t *errmsg = NULL;
    _cc_assert(ctx != NULL);
    if (ctx->transaction) {
        return true;
    }

    res = _sqlite3_exec(ctx->sql, "BEGIN", NULL, NULL, &errmsg);

    if (res != SQLITE_OK) {
        _cc_logger_error(_T("_sqlite_begin_transaction %s"), errmsg);
        _sqlite3_free(errmsg);
        return false;
    }
    ctx->transaction = true;
    return true;
}

static bool_t _sqlite_commit(_cc_sql_t *ctx) {
    int res = 0;
    char_t *errmsg = NULL;

    if (ctx->transaction == false) {
        return true;
    }

    _cc_assert(ctx != NULL);
    res = _sqlite3_exec(ctx->sql, "COMMIT", NULL, NULL, &errmsg);
    if (res != SQLITE_OK) {
        _cc_logger_error(_T("_sqlite_commit %s"), errmsg);
        _sqlite3_free(errmsg);
        return false;
    }
    ctx->transaction = false;
    return true;
}

static bool_t _sqlite_rollback(_cc_sql_t *ctx) {
    int res;
    char_t *errmsg = NULL;
    _cc_assert(ctx != NULL);

    if (ctx->transaction == false) {
        return true;
    }

    res = _sqlite3_exec(ctx->sql, "ROLLBACK", NULL, NULL, &errmsg);
    if (res != SQLITE_OK) {
        _cc_logger_error(_T("_sqlite_rollback %s"), errmsg);
        _sqlite3_free(errmsg);
        return false;
    }
    ctx->transaction = false;
    return true;
}

static bool_t _sqlite_fetch(_cc_sql_result_t *result) {
    int res = SQLITE_ERROR;
    _cc_assert(result != NULL && result->stmt != NULL);

    if (result->step_status != SQLITE_DONE && result->step_status != SQLITE_ROW) {
        res = _sqlite3_step(result->stmt);
        if ((res != SQLITE_OK) && (res != SQLITE_DONE) && (res != SQLITE_ROW)) {
            return false;
        }
    }
    
    result->step_status = SQLITE_OK;
    return (SQLITE_ROW == res);
}

static uint64_t _sqlite_get_num_rows(_cc_sql_result_t *result) {
    _cc_assert(result != NULL && result->stmt != NULL);
    _cc_logger_debug(_T("SQLite3 get_num_rows: Not implemented yet"));
    return 0;
}

static int32_t _sqlite_get_num_fields(_cc_sql_result_t *result) {
    if (result == NULL || result->stmt == NULL) {
        return 0;
    }
    return _sqlite3_column_count(result->stmt);
}

static bool_t _sqlite_next_result(_cc_sql_t *ctx, _cc_sql_result_t *result) {
    int res = SQLITE_ERROR;
    _cc_assert(ctx != NULL && result != NULL && result->stmt != NULL);

    res = _sqlite3_step(result->stmt);
    if ((res != SQLITE_OK) && (res != SQLITE_DONE) && (res != SQLITE_ROW)) {
        _cc_logger_error(_T("_sqlite3_step fail:%s"), _sqlite3_errmsg(ctx->sql));
        return false;
    }

    result->step_status = res;
    return (SQLITE_ROW == res);
}

static bool_t _sqlite_free_result(_cc_sql_t *ctx, _cc_sql_result_t *result) {
    int res = 0;
    _cc_assert(ctx != NULL && result != NULL);

    res = _sqlite3_finalize(result->stmt);
    if (res != SQLITE_OK) {
        _cc_logger_error(_T("_sqlite3_finalize fail:%s"), _sqlite3_errmsg(ctx->sql));
    }

    _cc_free(result);
    return true;
}

/**/
static bool_t _sqlite_bind(_cc_sql_result_t *result, int32_t index, const void *value, size_t length, _sql_enum_field_types_t type) {
    int res;
    _cc_assert(result != NULL);

    index++;
    switch(type) {
        case _CC_SQL_TYPE_INT8_:
            res = _sqlite3_bind_int(result->stmt, index, (int8_t)*(int8_t*)value);
            break;
        case _CC_SQL_TYPE_INT16_:
            res = _sqlite3_bind_int(result->stmt, index, (int16_t)*(int16_t*)value);
            break;
        case _CC_SQL_TYPE_INT32_:
            res = _sqlite3_bind_int(result->stmt, index, (int32_t)*(int32_t*)value);
            break;
        case _CC_SQL_TYPE_UINT8_:
            res = _sqlite3_bind_int(result->stmt, index, (uint8_t)*(uint8_t*)value);
            break;
        case _CC_SQL_TYPE_UINT16_:
            res = _sqlite3_bind_int(result->stmt, index, (uint16_t)*(uint16_t*)value);
            break;
        case _CC_SQL_TYPE_UINT32_:
        case _CC_SQL_TYPE_TIMESTAMP_:
            res = _sqlite3_bind_int(result->stmt, index, (uint32_t)*(uint32_t*)value);
            break;
        case _CC_SQL_TYPE_INT64_:
            res = _sqlite3_bind_int64(result->stmt, index, (int64_t)*(int64_t*)value);
            break;
        case _CC_SQL_TYPE_UINT64_:
            res = _sqlite3_bind_int64(result->stmt, index, (uint64_t)*(uint64_t*)value);
            break;
        case _CC_SQL_TYPE_FLOAT_:
            res = _sqlite3_bind_double(result->stmt, index, (float)*(float*)value);
            break;
        case _CC_SQL_TYPE_DOUBLE_:
            res = _sqlite3_bind_double(result->stmt, index, (double)*(double*)value);
            break;
        case _CC_SQL_TYPE_STRING_:
        case _CC_SQL_TYPE_JSON_:
            res = _sqlite3_bind_text(result->stmt, index, value, (int)length, SQLITE_TRANSIENT);
            break;
        case _CC_SQL_TYPE_DATETIME_: {
            tchar_t datetime[32];
            struct tm *timeinfo = (struct tm *)value;
            length = _sntprintf(datetime,_cc_countof(datetime), _T("%4d-%02d-%02d %02d:%02d:%02d"), 
                                timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, 
                                timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
            res = _sqlite3_bind_text(result->stmt, index, datetime, (int)length, SQLITE_TRANSIENT);
            break;
        }
        case _CC_SQL_TYPE_BLOB_:
            res = sqlite3_bind_blob(result->stmt, index, value, (int)length, SQLITE_STATIC);
            break;
        case _CC_SQL_TYPE_NULL_:
            res = sqlite3_bind_null(result->stmt, index);
            break;
        default:
            return false;
    }
    if (SQLITE_OK != res) {
        _cc_logger_error(_T("sqlite3_bind_int %s"), _sqlite3_errmsg(sqlite3_db_handle(result->stmt)));
        return false;
    }
    return true;
}

/**/
static uint64_t _sqlite_get_last_id(_cc_sql_t *ctx, _cc_sql_result_t *result) {
    _cc_assert(ctx != NULL && ctx->sql != NULL);
    return (uint64_t)sqlite3_last_insert_rowid(ctx->sql);
}

static pvoid_t _sqlite_get_stmt(_cc_sql_result_t *result) {
    return result->stmt;
}

static int32_t _sqlite_get_int(_cc_sql_result_t *result, int32_t index) {
    _cc_assert(result->stmt != NULL);
    return _sqlite3_column_int(result->stmt, index);
}

static int64_t _sqlite_get_int64(_cc_sql_result_t *result, int32_t index) {
    _cc_assert(result->stmt != NULL);
    return _sqlite3_column_int64(result->stmt, index);
}

static float64_t _sqlite_get_float(_cc_sql_result_t *result, int32_t index) {
    _cc_assert(result->stmt != NULL);
    return _sqlite3_column_double(result->stmt, index);
}

static size_t _sqlite_get_string(_cc_sql_result_t *result, int32_t index, tchar_t *buffer, size_t length) {
    size_t bytes_length;
    const tchar_t *v = (const tchar_t *)_sqlite3_column_text(result->stmt, index);
    *buffer = 0;
    if (v == NULL) {
        return 0;
    }

    bytes_length = _sqlite3_column_bytes(result->stmt, index);
    if (bytes_length == 0) {
        return 0;
    }

    if (bytes_length >= length) {
        bytes_length = length - 1;
    }

    _tcsncpy(buffer, v, bytes_length);
    buffer[bytes_length] = 0;
    return bytes_length;
}

static size_t _sqlite_get_blob(_cc_sql_result_t *result, int32_t index, byte_t **value) {
    _cc_assert(result->stmt != NULL);
    if (value) {
        *value = (byte_t*)_sqlite3_column_blob(result->stmt, index);
    }
    return _sqlite3_column_bytes(result->stmt, index);
}

static struct tm* _sqlite_get_datetime(_cc_sql_result_t *result, int32_t index) {
    const tchar_t *v;
    static struct tm timeinfo = {0};
    _cc_assert(result->stmt != NULL);

    v = (const tchar_t *)_sqlite3_column_text(result->stmt, index);
    if (v) {
        _cc_strptime(v, _T("%Y-%m-%d %H:%M:%S"), &timeinfo);
        return &timeinfo;
    }
    return &timeinfo;
}

/**/
bool_t _cc_init_sqlite(_cc_sql_driver_t *driver) {
#define SET(x) driver->x = _sqlite_##x

    if (_cc_unlikely(driver == NULL)) {
        return false;
    }

    /**/
    SET(connect);
    SET(disconnect);
    SET(prepare);
    SET(reset);
    SET(step);
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
