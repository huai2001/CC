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
#include <cc/db/sql.h>
#include <cc/socket/socket.h>
#include <cc/string.h>
#include <cc/time.h>
#include <cc/math.h>

#if defined(__CC_WINDOWS__) || defined(__CC_APPLE__)
#include <mysql/include/errmsg.h>
#include <mysql/include/mysql.h>
#include <mysql/include/mysqld_error.h>
#else
#include <mysql/errmsg.h>
#include <mysql/mysql.h>
#include <mysql/mysqld_error.h>
#endif

struct _cc_sql {
    MYSQL *sql;
    /* 0 for manual commit */
    bool_t auto_commit;

    bool_t use_SSL;
    uint16_t port;
    char_t charset[32];
    char_t db_name[64];
    char_t host[64];
    char_t user_name[64];
    char_t user_pass[64];

#ifdef _CC_UNICODE_
    _cc_buf_t buffer;
#endif
};

struct _cc_sql_result {
    MYSQL_STMT *stmt;
    MYSQL_BIND *binds;
    MYSQL_BIND *bind_result;
    MYSQL_RES *meta;
    int32_t num_fields;
    int32_t bind_fields;
#if DEBUG_EXECUTION_TIME
    clock_t execution_time;
#endif
};

static bool_t _get_url_query(const _cc_str_t *keyword, const tchar_t *p, char_t *buf, int32_t length) {
    int i = 0;
    tchar_t value[128];
    const tchar_t *r = _tcsstr(p, keyword->data);
    if (r == NULL) {
        return false;
    }

    r += keyword->length;
    for (i = 0; *r && *r != _T('&'); i++) {
        value[i] = *r++;
        if (i >= length) {
            break;
        }
    }
    value[i] = 0;

#ifdef _CC_UNICODE_
    _cc_utf16_to_utf8((uint16_t *)value, (uint16_t *)(value + i),(uint8_t *)buf, (uint8_t *)(buf + length), false);
#else
    _tcsncpy(buf, value, length);
    buf[length - 1] = 0;
#endif
    return true;
}
static bool_t _mysql_reconnect(_cc_sql_t *ctx);
static bool_t _mysql_error(_cc_sql_t *ctx) {
    int sql_errno = mysql_errno(ctx->sql);
    switch (sql_errno) {
    case CR_SERVER_GONE_ERROR:
    case CR_SERVER_LOST:
    case CR_SERVER_LOST_EXTENDED:
        if (ctx->sql) {
            mysql_close(ctx->sql);
            ctx->sql = NULL;
        }
    case CR_CONN_HOST_ERROR:
        return _mysql_reconnect(ctx);
    case ER_LOCK_DEADLOCK:
    case ER_WRONG_VALUE_COUNT:
    case ER_DUP_ENTRY:
        break;
    case ER_BAD_FIELD_ERROR:
    case ER_NO_SUCH_TABLE:
        _cc_logger_error(_T("Your database structure is not up to date. Please make ")
                         _T("sure you've executed all queries in the sql/updates ")
                         _T("folders. %u: %s."),
                         sql_errno, mysql_error(ctx->sql));
        break;
    case ER_PARSE_ERROR:
        _cc_logger_error(_T("Error while parsing SQL. %u: %s."), sql_errno, mysql_error(ctx->sql));
        break;
    default:
        _cc_logger_error(_T("Unhandled MySQL errno %u: %s."), sql_errno, mysql_error(ctx->sql));
        break;
    }
    return false;
}

static bool_t _mysql_reconnect(_cc_sql_t *ctx) {
    MYSQL *res = NULL;
    char value = 1;
    char *charset;
    ctx->sql = mysql_init(NULL);
    if (_cc_unlikely(ctx->sql == NULL)) {
        _cc_logger_error(_T("Could not initialize Mysql connection to database `%s`"), ctx->host);
        return false;
    }
    
    if (ctx->use_SSL) {
#if !defined(MARIADB_VERSION_ID) && MYSQL_VERSION_ID >= 80000
        int opt_use_ssl = SSL_MODE_REQUIRED;
        mysql_options(ctx->sql, MYSQL_OPT_SSL_MODE, (void const *)&opt_use_ssl);
#elif defined(MYSQL_OPT_SSL_ENFORCE)
        my_bool opt_use_ssl = 1;
        mysql_options(ctx->sql, MYSQL_OPT_SSL_ENFORCE, (void const *)&opt_use_ssl);
#endif
    }

    res = mysql_real_connect(ctx->sql, ctx->host, ctx->user_name, ctx->user_pass, ctx->db_name, ctx->port, NULL, 0);
    if (_cc_unlikely(res == NULL)) {
        _cc_logger_error(_T("Connection error %d: %s"), mysql_errno(ctx->sql), mysql_error(ctx->sql));
        return false;
    }

    mysql_options(ctx->sql, MYSQL_OPT_RECONNECT, &value);

    charset = ctx->charset[0] == 0 ? "utf8mb4" : ctx->charset;
#if (MYSQL_VERSION_ID > 41000)
    {
        char_t buf[512];
        size_t len = snprintf(buf, _cc_countof(buf), "SET character_set_connection=%s, character_set_results=%s, character_set_client=binary", charset, charset);
        mysql_real_query(ctx->sql, buf, (unsigned long)len);
    }
#else
    if (mysql_set_character_set(ctx->sql, charset)) {
        _cc_logger_error(_T("mysql_set_character_set error %d: %s"), mysql_errno(ctx->sql), mysql_error(ctx->sql));
    }
#endif

#ifdef _CC_UNICODE_
    _cc_buf_alloc(&ctx->buffer,1024);
#endif
    ctx->auto_commit = true;

    return true;
}

static _cc_sql_t *_mysql_connect(const tchar_t *sql_connection_string) {
    _cc_sql_t *ctx = NULL;
    _cc_url_t params;
    static _cc_str_t charset_attr = _cc_string(_T("charset="));
    static _cc_str_t SSL_attr = _cc_string(_T("SSL="));

    if (!_cc_parse_url(&params, sql_connection_string)) {
        return NULL;
    }

    ctx = (_cc_sql_t *)_cc_malloc(sizeof(_cc_sql_t));
    bzero(ctx, sizeof(_cc_sql_t));
#ifdef _CC_UNICODE_
    _cc_utf16_to_utf8((uint16_t *)(params.path + 1), (uint16_t *)((params.path + 1) + _tcslen((params.path + 1))),
                      (uint8_t *)ctx->db_name, (uint8_t *)&ctx->db_name[64], false);
    _cc_utf16_to_utf8((uint16_t *)params.host, (uint16_t *)(params.host + _tcslen(params.host)), (uint8_t *)ctx->host,
                      (uint8_t *)&ctx->host[64], false);
    _cc_utf16_to_utf8((uint16_t *)params.username, (uint16_t *)(params.username + _tcslen(params.username)),
                      (uint8_t *)ctx->user_name, (uint8_t *)&ctx->user_name[64], false);
    _cc_utf16_to_utf8((uint16_t *)params.password, (uint16_t *)(params.password + _tcslen(params.password)),
                      (uint8_t *)ctx->user_pass, (uint8_t *)&ctx->user_pass[64], false);
#else
    _tcsncpy(ctx->db_name, (params.path + 1), _cc_countof(ctx->db_name));
    _tcsncpy(ctx->host, (params.host), _cc_countof(ctx->host));
    _tcsncpy(ctx->user_name, (params.username), _cc_countof(ctx->user_name));
    _tcsncpy(ctx->user_pass, (params.password), _cc_countof(ctx->user_pass));
    ctx->db_name[_cc_countof(ctx->db_name) - 1] = 0;
    ctx->host[_cc_countof(ctx->host) - 1] = 0;
    ctx->user_name[_cc_countof(ctx->user_name) - 1] = 0;
    ctx->user_pass[_cc_countof(ctx->user_pass) - 1] = 0;
#endif
    ctx->port = params.port;
    ctx->use_SSL = false;
    ctx->charset[0] = 0;

    if (params.query) {
        const tchar_t *r;
        _get_url_query(&charset_attr, params.query, ctx->charset, _cc_countof(ctx->charset));
        r = _tcsstr(params.query, SSL_attr.data);
        if (r && _tcsnicmp(r + SSL_attr.length, _T("true"), 4) == 0) {
            ctx->use_SSL = true;
        }
    }

    _cc_free_url(&params);

    if (_mysql_reconnect(ctx)) {
        return ctx;
    }

    _cc_free(ctx);
    return NULL;
}

static bool_t _mysql_disconnect(_cc_sql_t *ctx) {
#ifdef _CC_UNICODE_
    _cc_buf_free(&ctx->buffer);
#endif
    mysql_close(ctx->sql);
    
    _cc_free(ctx);
    return true;
}

static bool_t _cc_mysql_prepare(_cc_sql_t *ctx, MYSQL_STMT *stmt, const char_t *sql_string, unsigned long sql_string_len) {
    if (mysql_stmt_prepare(stmt, sql_string, sql_string_len)) {
        if (_mysql_error(ctx)) {
            if (mysql_stmt_prepare(stmt, sql_string, sql_string_len)) {
                return false;
            }
            return true;
        }
        return false;
    }
    return true;
}

static bool_t _cc_mysql_query(_cc_sql_t *ctx, const char_t *sql_string, unsigned long sql_string_len) {
    if (mysql_real_query(ctx->sql, sql_string, sql_string_len)) {
        if (_mysql_error(ctx)) {
            if (mysql_real_query(ctx->sql, sql_string, sql_string_len)) {
                return false;
            }
            return true;
        }
        return false;
    }
    return true;
}

/**/
static bool_t _mysql_auto_commit(_cc_sql_t *ctx, bool_t is_auto_commit) {
    _cc_assert(ctx != NULL);
    ctx->auto_commit = is_auto_commit;
    /* Set it ON /OFF */
    mysql_autocommit(ctx->sql, is_auto_commit);

    return true;
}

/**/
static bool_t _mysql_begin_transaction(_cc_sql_t *ctx) {
    _cc_assert(ctx != NULL);
    if (ctx->auto_commit) {
        return _cc_mysql_query(ctx, "start transaction;", 18);
    }

    return true;
}

/**/
static bool_t _mysql_commit(_cc_sql_t *ctx) {
    _cc_assert(ctx != NULL);
    return mysql_commit(ctx->sql);
}

/**/
static bool_t _mysql_rollback(_cc_sql_t *ctx) {
    _cc_assert(ctx != NULL);
    return mysql_rollback(ctx->sql);
}

/**/
static bool_t __bind_result(_cc_sql_result_t *result) {
    MYSQL_RES* res;
    int32_t num_fields;
    int32_t i;
    
    res = mysql_stmt_result_metadata(result->stmt);
    if (res == NULL) {
        return false;
    }
    num_fields = (int32_t)mysql_stmt_field_count(result->stmt);
    if (num_fields > 0) {
        MYSQL_FIELD *fields = mysql_fetch_fields(res);
        MYSQL_BIND *binds = (MYSQL_BIND*)_cc_calloc(num_fields + 1, sizeof(MYSQL_BIND));
        for (i = 0; i < num_fields; i++) {
            MYSQL_FIELD *field = &fields[i];
            MYSQL_BIND *b = &binds[i];
            b->buffer_type = field->type;
            /**
             * An exception occurs on mysql_close
             * malloc: Incorrect checksum for freed object 0x7fe46762aa60: probably modified after being freed.
             */
            switch (field->type) {
                case MYSQL_TYPE_DATETIME:
                case MYSQL_TYPE_DATE:
                case MYSQL_TYPE_TIME:
                    b->buffer_length = sizeof(MYSQL_TIME);
                    break;
                default:
                    b->buffer_length = field->length;
                    break;
            }
            b->buffer = _cc_malloc(b->buffer_length);
        }

        mysql_stmt_bind_result(result->stmt, binds);
        result->bind_result = binds;
    } else {
        result->bind_result = NULL;
    }
    
    result->meta = res;
    result->num_fields = num_fields;
    mysql_stmt_store_result(result->stmt);
#if DEBUG_EXECUTION_TIME
    result->execution_time = clock() - result->execution_time;
    if (result->execution_time > 500) {
        _cc_logger_warin(_T("MySQL query took %ld ms"), result->execution_time / CLOCKS_PER_SEC);
    }
#endif
    return true;
}

/**/
static void __free_bind_result(_cc_sql_result_t *result) {
    int32_t i;
    if (result->meta) {
        mysql_free_result(result->meta);
        result->meta = NULL;
    }
    
    if (result->bind_result) {
        for (i = 0; i < result->num_fields; i++) {
            MYSQL_BIND *b = &result->bind_result[i];
            _cc_free(b->buffer);
        }
        _cc_free(result->bind_result);
        result->bind_result = NULL;
    }
}

static bool_t _mysql_prepare(_cc_sql_t *ctx, const tchar_t *sql_string, _cc_sql_result_t **result) {
    size_t sql_string_len = 0;
    int32_t bind_fields;
    const char* ptr;
    MYSQL_STMT *stmt = NULL;
#if DEBUG_EXECUTION_TIME
    clock_t execution_time = clock();
#endif
    _cc_assert(ctx != NULL && ctx->sql != NULL);
    stmt = mysql_stmt_init(ctx->sql);
    if (stmt == NULL) {
        _cc_logger_error(_T("mysql_stmt_init error %d: %s"), mysql_errno(ctx->sql), mysql_error(ctx->sql));
        return false;
    }

    sql_string_len = (int32_t)_tcslen(sql_string);
#ifdef _CC_UNICODE_
    _cc_buf_cleanup(&ctx->buffer);
    _cc_buf_write(&ctx->buffer, sql_string, sql_string_len);
    _cc_buf_utf16_to_utf8(&ctx->buffer, 0);
    ptr = (const char*)ctx->buffer.bytes;
    sql_string_len = ctx->buffer.length / sizeof(char_t);
#else
    ptr = sql_string;
#endif

    if (!_cc_mysql_prepare(ctx, stmt, ptr, (unsigned long)sql_string_len)) {
        return false;
    }

    bind_fields = (int32_t)mysql_stmt_param_count(stmt);
    if (result) {
        _cc_sql_result_t *res = (_cc_sql_result_t *)_cc_malloc(sizeof(_cc_sql_result_t));
        bzero(res, sizeof(_cc_sql_result_t));
        res->stmt = stmt;
        res->meta = NULL;
        res->num_fields = 0;
        res->bind_fields = bind_fields;
        if (bind_fields > 0) {
            res->binds = (MYSQL_BIND*)_cc_calloc(bind_fields, sizeof(MYSQL_BIND));
        }
#if DEBUG_EXECUTION_TIME
        res->execution_time = execution_time;
#endif
        *result = res;
        return true;
    }

    mysql_stmt_close(stmt);
    return false;
}

static bool_t _mysql_reset(_cc_sql_t *ctx, _cc_sql_result_t *result) {
    if (result->stmt == NULL) {
        return false;
    }

    mysql_stmt_reset(result->stmt);
    bzero(result->binds, sizeof(MYSQL_BIND) * result->bind_fields);
    return true;
}

static bool_t _mysql_step(_cc_sql_t *ctx, _cc_sql_result_t *result) {
    if (result->stmt == NULL) {
        return false;
    }
    
    if (result->binds) {
        if (mysql_stmt_bind_param(result->stmt, result->binds) != 0) {
            _cc_logger_error(_T("mysql_stmt_execute error %d: %s"), mysql_errno(ctx->sql), mysql_error(ctx->sql));
            return false;
        }
    }
    
    if (mysql_stmt_execute(result->stmt) != 0) {
        _cc_logger_error(_T("mysql_stmt_execute error %d: %s"), mysql_errno(ctx->sql), mysql_error(ctx->sql));
        return false;
    }
    
    return __bind_result(result);
}
static bool_t _mysql_execute(_cc_sql_t *ctx, const tchar_t *sql_string, _cc_sql_result_t **result) {
    size_t sql_string_len = 0;
    const char_t *ptr = NULL;
    int32_t bind_fields;
    _cc_sql_result_t *res;
    MYSQL_STMT *stmt = NULL;
#if DEBUG_EXECUTION_TIME
    clock_t execution_time = clock();
#endif
    _cc_assert(ctx != NULL && sql_string != NULL);

    sql_string_len = (int32_t)_tcslen(sql_string);

#ifdef _CC_UNICODE_
    _cc_buf_cleanup(&ctx->buffer);
    _cc_buf_write(&ctx->buffer, sql_string, sql_string_len);
    _cc_buf_utf16_to_utf8(&ctx->buffer, 0);
    ptr = ctx->buffer.bytes;
    sql_string_len = ctx->buffer.length / sizeof(char_t);
#else
    ptr = sql_string;
#endif
    if (result == NULL) {
        return _cc_mysql_query(ctx, ptr, (unsigned long)sql_string_len);
    }
    
    stmt = mysql_stmt_init(ctx->sql);
    if (stmt == NULL) {
        _cc_logger_error(_T("mysql_stmt_init error %d: %s"), mysql_errno(ctx->sql), mysql_error(ctx->sql));
        return false;
    }
    
    if (!_cc_mysql_prepare(ctx, stmt, ptr, (unsigned long)sql_string_len)) {
        return false;
    }

    bind_fields = (int32_t)mysql_stmt_param_count(stmt);
    res = (_cc_sql_result_t *)_cc_malloc(sizeof(_cc_sql_result_t));
    bzero(res, sizeof(_cc_sql_result_t));
    res->stmt = stmt;
    res->binds = NULL;
    res->bind_result = NULL;
    res->meta = NULL;
    res->num_fields = 0;
    res->bind_fields = bind_fields;
    if (bind_fields > 0) {
        res->binds = (MYSQL_BIND*)_cc_calloc(bind_fields, sizeof(MYSQL_BIND));
    }
#if DEBUG_EXECUTION_TIME
    res->execution_time = execution_time;
#endif
    *result = res;
    return _mysql_step(ctx, res);
}
/**/
static bool_t _mysql_next_result(_cc_sql_t *ctx, _cc_sql_result_t *result) {
    _cc_assert(ctx != NULL && result != NULL);

    __free_bind_result(result);

    if (!mysql_stmt_next_result(result->stmt)) {
        return false;
    }

    return __bind_result(result);
}

static bool_t _mysql_fetch(_cc_sql_result_t *result) {
    _cc_assert(result != NULL && result->stmt != NULL);

    return mysql_stmt_fetch(result->stmt) == 0;
}

static uint64_t _mysql_get_num_rows(_cc_sql_result_t *result) {
    _cc_assert(result != NULL && result->stmt != NULL);
    return mysql_stmt_num_rows(result->stmt);
}

static int32_t _mysql_get_num_fields(_cc_sql_result_t *result) {
    _cc_assert(result != NULL && result->stmt != NULL);
    return result->num_fields;
}

static bool_t _mysql_free_result(_cc_sql_t *ctx, _cc_sql_result_t *result) {
    _cc_assert(result != NULL);
    __free_bind_result(result);
    
    if (result->binds) {
        _cc_free(result->binds);
    }
    
    if (result->stmt) {
        mysql_stmt_close(result->stmt);
        result->stmt = NULL;
    }
    _cc_free(result);

    return true;
}

/**/
static bool_t _mysql_bind(_cc_sql_result_t *result, int32_t index, const void *value, size_t length, _sql_enum_field_types_t type) {
    MYSQL_BIND *b;
    _cc_assert(result != NULL);
    if (index >= result->bind_fields) {
        return false;
    }

    b = &result->binds[index];
    b->buffer = (void*)value;
    b->buffer_length = (unsigned long)length;

    switch(type) {
        case _CC_SQL_TYPE_INT8_:
            b->is_unsigned = false;
            b->buffer_type = MYSQL_TYPE_TINY;
            break;
        case _CC_SQL_TYPE_INT16_:
            b->is_unsigned = false;
            b->buffer_type = MYSQL_TYPE_SHORT;
            break;
        case _CC_SQL_TYPE_INT32_:
            b->is_unsigned = false;
            b->buffer_type = MYSQL_TYPE_LONG;
            break;
        case _CC_SQL_TYPE_INT64_:
            b->is_unsigned = false;
            b->buffer_type = MYSQL_TYPE_LONGLONG;
            break;
        case _CC_SQL_TYPE_UINT8_:
            b->is_unsigned = true;
            b->buffer_type = MYSQL_TYPE_TINY;
            break;
        case _CC_SQL_TYPE_UINT16_:
            b->is_unsigned = true;
            b->buffer_type = MYSQL_TYPE_SHORT;
            break;
        case _CC_SQL_TYPE_UINT32_:
            b->is_unsigned = true;
            b->buffer_type = MYSQL_TYPE_LONG;
            break;
        case _CC_SQL_TYPE_UINT64_:
            b->is_unsigned = true;
            b->buffer_type = MYSQL_TYPE_LONGLONG;
            break;
        case _CC_SQL_TYPE_FLOAT_:
            b->is_unsigned = false;
            b->buffer_type = MYSQL_TYPE_FLOAT;
            break;
        case _CC_SQL_TYPE_DOUBLE_:
            b->is_unsigned = false;
            b->buffer_type = MYSQL_TYPE_DOUBLE;
            break;
        case _CC_SQL_TYPE_STRING_: {
            b->is_unsigned = false;
            b->buffer_type = MYSQL_TYPE_STRING;
            if (length == -1) {
                b->buffer_length = (unsigned long)_tcslen((tchar_t*)value);
            }
        }
            break;
        case _CC_SQL_TYPE_BLOB_:
            b->is_unsigned = false;
            b->buffer_type = MYSQL_TYPE_BLOB;
            break;
        case _CC_SQL_TYPE_NULL_:
            b->is_unsigned = false;
            b->buffer_type = MYSQL_TYPE_BLOB;
            break;
        case _CC_SQL_TYPE_DATETIME_: {
            struct tm *timeinfo = (struct tm*)value;
            MYSQL_TIME *datetime = (MYSQL_TIME*)b->buffer;
            b->buffer_type = MYSQL_TYPE_DATETIME;
            
            datetime->year = timeinfo->tm_year + 1900;
            datetime->month = timeinfo->tm_mon + 1;
            datetime->day = timeinfo->tm_mday;
            datetime->hour = timeinfo->tm_hour;
            datetime->minute = timeinfo->tm_min;
            datetime->second = timeinfo->tm_sec;
        }
            break;
        case _CC_SQL_TYPE_TIMESTAMP_:
            b->is_unsigned = false;
            b->buffer_type = MYSQL_TYPE_TIMESTAMP;
            break;
        case _CC_SQL_TYPE_JSON_: {
            b->is_unsigned = false;
            b->buffer_type = MYSQL_TYPE_JSON;
            if (length == -1) {
				b->buffer_length = (unsigned long)_tcslen((tchar_t*)value);
            }
        }
            break;
    }
    return true;
}

/**/
static uint64_t _mysql_get_last_id(_cc_sql_t *ctx, _cc_sql_result_t *result) {
    _cc_assert(result != NULL && result->stmt != NULL);
    return mysql_stmt_insert_id(result->stmt);
}

static pvoid_t _mysql_get_stmt(_cc_sql_result_t *result) {
    return result->stmt;
}

/**/
static int32_t _mysql_get_int(_cc_sql_result_t *result, int32_t index) {
    MYSQL_BIND *b;
    _cc_assert(result != NULL);
    if (index >= result->num_fields) {
        return 0;
    }
    b = &result->bind_result[index];
    return *(int32_t*)(b->buffer);
}

/**/
static int64_t _mysql_get_int64(_cc_sql_result_t *result, int32_t index) {
    MYSQL_BIND *b;
    _cc_assert(result != NULL);
    if (index >= result->num_fields) {
        return 0;
    }

    b = &result->bind_result[index];
    return *(int64_t*)(b->buffer);
}

/**/
static float64_t _mysql_get_float(_cc_sql_result_t *result, int32_t index) {
    MYSQL_BIND *b;
    _cc_assert(result != NULL);
    if (index >= result->num_fields) {
        return false;
    }
    b = &result->bind_result[index];
    return *(float64_t*)(b->buffer);
}

/**/
static size_t _mysql_get_string(_cc_sql_result_t *result, int32_t index, tchar_t *buffer, size_t length) {
    MYSQL_BIND *b;
    size_t bytes_length;
    _cc_assert(result != NULL);
    if (index >= result->num_fields) {
        return 0;
    }
    *buffer = 0;

    b = &result->bind_result[index];
    bytes_length = _tcslen((tchar_t*)b->buffer);
    if (bytes_length == 0) {
        return 0;
    }

    if (bytes_length >= length) {
        bytes_length = length - 1;
    }

    _tcsncpy(buffer, b->buffer, bytes_length);
    buffer[bytes_length] = 0;
    return bytes_length;
}

/**/
static size_t _mysql_get_blob(_cc_sql_result_t *result, int32_t index, byte_t **buffer) {
    MYSQL_BIND *b;
    _cc_assert(result != NULL);
    if (index >= result->num_fields) {
        return false;
    }

    b = &result->bind_result[index];
    if (buffer) {
        *buffer = (byte_t*)b->buffer;
    }

    return b->buffer_length;
}

/**/
static struct tm* _mysql_get_datetime(_cc_sql_result_t *result, int32_t index) {
    MYSQL_BIND *b;
    MYSQL_TIME *datetime;
    static struct tm timeinfo = {0};
    _cc_assert(result != NULL);
    if (index >= result->num_fields) {
        return &timeinfo;
    }
    b = &result->bind_result[index];
    switch (b->buffer_type) {
        case MYSQL_TYPE_DATETIME:
        case MYSQL_TYPE_DATE:
        case MYSQL_TYPE_TIME:{
            datetime = (MYSQL_TIME*)(b->buffer);
            timeinfo.tm_year = 0;
            timeinfo.tm_mon = 0;
            if (datetime->year >= 1900) {
                timeinfo.tm_year = datetime->year - 1900;
            }
            if (datetime->month >= 1) {
                timeinfo.tm_mon = datetime->month - 1;
            }
            timeinfo.tm_mday = datetime->day;
            timeinfo.tm_hour = datetime->hour;
            timeinfo.tm_min = datetime->minute;
            timeinfo.tm_sec = datetime->second;
            return &timeinfo;
        }
        default:
            return &timeinfo;
    }
}
/**/
bool_t _cc_init_mysql(_cc_sql_driver_t *driver) {
#define SET(x) driver->x = _mysql_##x

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
