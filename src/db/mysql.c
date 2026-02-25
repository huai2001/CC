#include <libcc/alloc.h>
#include <libcc/math.h>
#include <libcc/socket.h>
#include <libcc/string.h>
#include <libcc/time.h>
#include <libcc/sql.h>
#include <libcc/url.h>

#if defined(__CC_WINDOWS__)
#include <mysql/include/mysqld_error.h>
#include <mysql/include/mysql.h>
#else
#include <mysql/mysqld_error.h>
#include <mysql/mysql.h>
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
    bool_t step;
    int32_t num_of_dataset;
    int32_t num_of_bind;
    MYSQL_STMT *stmt;
    MYSQL_BIND *binds;
    MYSQL_BIND *dataset;
    MYSQL_RES *meta;
#if DEBUG_EXECUTION_TIME
    clock_t execution_time;
#endif
};

_CC_API_PRIVATE(bool_t) _mysql_reconnect(_cc_sql_t *ctx);
_CC_API_PRIVATE(bool_t) _mysql_error(_cc_sql_t *ctx) {
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
        _cc_logger_error("Your database structure is not up to date. Please make "
                         _T("sure you've executed all queries in the sql/updates ")
                         _T("folders. %u: %s."),
                         sql_errno, mysql_error(ctx->sql));
        break;
    case ER_PARSE_ERROR:
        _cc_logger_error("Error while parsing SQL. %u: %s.", sql_errno, mysql_error(ctx->sql));
        break;
    default:
        _cc_logger_error("Unhandled MySQL errno %u: %s.", sql_errno, mysql_error(ctx->sql));
        break;
    }
    return false;
}

_CC_API_PRIVATE(bool_t) _mysql_reconnect(_cc_sql_t *ctx) {
    MYSQL *res = NULL;
    char *charset;
    ctx->sql = mysql_init(NULL);
    if (_cc_unlikely(ctx->sql == NULL)) {
        _cc_logger_error("Could not initialize Mysql connection to database `%s`", ctx->host);
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
        _cc_logger_error("Connection error %d: %s", mysql_errno(ctx->sql), mysql_error(ctx->sql));
        return false;
    }

    charset = ctx->charset[0] == 0 ? "utf8mb4" : ctx->charset;
#if (MYSQL_VERSION_ID > 41000)
    {
        char_t buf[128];
        size_t len = snprintf(buf, _cc_countof(buf),
                              "SET character_set_connection=%s, character_set_dataset=%s, character_set_client=binary",
                              charset, charset);
        mysql_real_query(ctx->sql, buf, (unsigned long)len);
    }
#else
    if (mysql_set_character_set(ctx->sql, charset)) {
        _cc_logger_error("mysql_set_character_set error %d: %s", mysql_errno(ctx->sql), mysql_error(ctx->sql));
    }
#endif

#ifdef _CC_UNICODE_
    _cc_alloc_buf(&ctx->buffer, 1024);
#endif
    ctx->auto_commit = true;

    return true;
}

_CC_API_PRIVATE(_cc_sql_t *) _mysql_connect(const tchar_t *sql_connection_string) {
    _cc_sql_t *ctx = NULL;
    _cc_url_t url;

    if (!_cc_parse_url(&url, sql_connection_string)) {
        return NULL;
    }

    ctx = (_cc_sql_t *)_cc_malloc(sizeof(_cc_sql_t));
    bzero(ctx, sizeof(_cc_sql_t));
#ifdef _CC_UNICODE_
    _cc_utf16_to_utf8((uint16_t *)(url.path + 1), (uint16_t *)((url.path + 1) + _tcslen((url.path + 1))),
                      (uint8_t *)ctx->db_name, (uint8_t *)&ctx->db_name[64]);
    _cc_utf16_to_utf8((uint16_t *)url.host, (uint16_t *)(url.host + _tcslen(url.host)),
                      (uint8_t *)ctx->host, (uint8_t *)&ctx->host[64]);
    _cc_utf16_to_utf8((uint16_t *)url.username, (uint16_t *)(url.username + _tcslen(url.username)),
                      (uint8_t *)ctx->user_name, (uint8_t *)&ctx->user_name[64]);
    _cc_utf16_to_utf8((uint16_t *)url.password, (uint16_t *)(url.password + _tcslen(url.password)),
                      (uint8_t *)ctx->user_pass, (uint8_t *)&ctx->user_pass[64]);
#else
    strncpy(ctx->db_name, (url.path + 1), _cc_countof(ctx->db_name));
    ctx->db_name[_cc_countof(ctx->db_name) - 1] = '\0';
    strncpy(ctx->host, (url.host), _cc_countof(ctx->host));
    ctx->host[_cc_countof(ctx->host) - 1] = '\0';
    strncpy(ctx->user_name, (url.username), _cc_countof(ctx->user_name));
    ctx->user_name[_cc_countof(ctx->user_name) - 1] = '\0';
    strncpy(ctx->user_pass, (url.password), _cc_countof(ctx->user_pass));
    ctx->user_pass[_cc_countof(ctx->user_pass) - 1] = '\0';
#endif
    ctx->port = url.port;
    ctx->use_SSL = false;
    ctx->charset[0] = 0;

    if (url.query) {
        const tchar_t *r;
        r = _tcsstr(url.query, _T("charset="));
        if (r) {
            int i = 0;
            r += 8;
            while (r && *r != '&' && i < _cc_countof(ctx->charset)) {
                ctx->charset[i++] = (char)*r++;
            }
            ctx->charset[i] = 0;
        }
        
        r = _tcsstr(url.query, _T("SSL="));
        if (r && _tcsnicmp(r + 5, _T("true"), 4) == 0) {
            ctx->use_SSL = true;
        }
    }

    _cc_free_url(&url);

    if (_mysql_reconnect(ctx)) {
        return ctx;
    }

    _cc_free(ctx);
    return NULL;
}

_CC_API_PRIVATE(bool_t) _mysql_disconnect(_cc_sql_t *ctx) {
#ifdef _CC_UNICODE_
    _cc_free_buf(&ctx->buffer);
#endif
    mysql_close(ctx->sql);

    _cc_free(ctx);
    return true;
}

_CC_API_PRIVATE(bool_t) _cc_mysql_prepare(_cc_sql_t *ctx, MYSQL_STMT *stmt, const char_t *sql_string, unsigned long sql_string_len) {
    if (mysql_stmt_prepare(stmt, sql_string, sql_string_len)) {
        if (_mysql_error(ctx)) {
            if (mysql_stmt_prepare(stmt, sql_string, sql_string_len)) {
                _cc_logger_error("mysql_stmt_prepare error %d: %s", mysql_stmt_errno(stmt), mysql_stmt_error(stmt));
                return false;
            }
            return true;
        }
        _cc_logger_error("mysql_stmt_prepare error %d: %s", mysql_stmt_errno(stmt), mysql_stmt_error(stmt));
        return false;
    }
    return true;
}

_CC_API_PRIVATE(bool_t) _cc_mysql_query(_cc_sql_t *ctx, const char_t *sql_string, unsigned long sql_string_len) {
    if (mysql_real_query(ctx->sql, sql_string, sql_string_len)) {
        if (_mysql_error(ctx)) {
            if (mysql_real_query(ctx->sql, sql_string, sql_string_len)) {
                _cc_logger_error("mysql_stmt_prepare error %d: %s", mysql_errno(ctx->sql), mysql_error(ctx->sql));
                return false;
            }
            return true;
        }
        _cc_logger_error("mysql_stmt_prepare error %d: %s", mysql_errno(ctx->sql), mysql_error(ctx->sql));
        return false;
    }
    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _mysql_auto_commit(_cc_sql_t *ctx, bool_t is_auto_commit) {
    _cc_assert(ctx != NULL);
    ctx->auto_commit = is_auto_commit;
    /* Set it ON /OFF */
    mysql_autocommit(ctx->sql, is_auto_commit);

    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _mysql_begin_transaction(_cc_sql_t *ctx) {
    _cc_assert(ctx != NULL);
    if (ctx->auto_commit) {
        return _cc_mysql_query(ctx, "start transaction;", 18);
    }

    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _mysql_commit(_cc_sql_t *ctx) {
    _cc_assert(ctx != NULL);
    if (mysql_commit(ctx->sql) != 0) {
        _cc_logger_error("mysql_commit error %d: %s", mysql_errno(ctx->sql), mysql_error(ctx->sql));
        return false;
    }
    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _mysql_rollback(_cc_sql_t *ctx) {
    _cc_assert(ctx != NULL);
    if (mysql_rollback(ctx->sql) != 0) {
        _cc_logger_error("mysql_commit error %d: %s", mysql_errno(ctx->sql), mysql_error(ctx->sql));
        return false;
    }
    return true;
}

/**/
_CC_API_PRIVATE(bool_t) __dataset(_cc_sql_result_t *result) {
    MYSQL_RES *res;
    int32_t num_of_dataset;
    int32_t i;

    res = mysql_stmt_result_metadata(result->stmt);
    if (res == NULL) {
        return mysql_stmt_affected_rows(result->stmt) > 0;
    }

    num_of_dataset = (int32_t)mysql_stmt_field_count(result->stmt);
    if (num_of_dataset > 0) {
        MYSQL_FIELD *fields = mysql_fetch_fields(res);
        MYSQL_BIND *dataset = (MYSQL_BIND *)_cc_calloc(num_of_dataset + 1, sizeof(MYSQL_BIND));
        for (i = 0; i < num_of_dataset; i++) {
            MYSQL_FIELD *field = &fields[i];
            MYSQL_BIND *b = &dataset[i];
            b->buffer_type = field->type;
            /**
             * An exception occurs on mysql_close
             * malloc: Incorrect checksum for freed object 0x7fe46762aa60: probably modified after being freed.
             */
            switch (field->type) {
            case MYSQL_TYPE_DATETIME:
            case MYSQL_TYPE_TIMESTAMP:
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

        mysql_stmt_bind_result(result->stmt, dataset);
        result->dataset = dataset;
    } else {
        result->dataset = NULL;
    }

    result->meta = res;
    result->num_of_dataset = num_of_dataset;
    mysql_stmt_store_result(result->stmt);
#if DEBUG_EXECUTION_TIME
    result->execution_time = clock() - result->execution_time;
    if (result->execution_time > 500) {
        _cc_logger_warin("MySQL query took %ld ms", result->execution_time / CLOCKS_PER_SEC);
    }
#endif
    return true;
}

/**/
_CC_API_PRIVATE(void) __free_dataset(_cc_sql_result_t *result) {
    int32_t i;
    if (result->meta) {
        mysql_free_result(result->meta);
        result->meta = NULL;
    }

    if (result->dataset) {
        for (i = 0; i < result->num_of_dataset; i++) {
            MYSQL_BIND *b = &result->dataset[i];
            _cc_free(b->buffer);
        }
        _cc_free(result->dataset);
        result->dataset = NULL;
    }
}

_CC_API_PRIVATE(bool_t) _mysql_execute(_cc_sql_t *ctx, const tchar_t *sql, size_t length, _cc_sql_result_t **result) {
    int32_t num_of_bind;
    MYSQL_STMT *stmt = NULL;
    _cc_sql_result_t *res;

#if DEBUG_EXECUTION_TIME
    clock_t execution_time = clock();
#endif

    _cc_assert(ctx != NULL && ctx->sql != NULL);

#ifdef _CC_UNICODE_
    _cc_buf_cleanup(&ctx->buffer);
    _cc_buf_append(&ctx->buffer, sql, length);
    _cc_buf_utf16_to_utf8(&ctx->buffer, 0);
    sql = (const char *)ctx->buffer.bytes;
    length = ctx->buffer.length / sizeof(char_t);
#endif

    if (result == NULL) {
        return _cc_mysql_query(ctx, sql, (unsigned long)length);
    }

    stmt = mysql_stmt_init(ctx->sql);
    if (stmt == NULL) {
        _cc_logger_error("mysql_stmt_init error %d: %s", mysql_errno(ctx->sql), mysql_error(ctx->sql));
        return false;
    }

    if (!_cc_mysql_prepare(ctx, stmt, sql, (unsigned long)length)) {
        return false;
    }

    num_of_bind = (int32_t)mysql_stmt_param_count(stmt);
    res = (_cc_sql_result_t *)_cc_malloc(sizeof(_cc_sql_result_t));
    bzero(res, sizeof(_cc_sql_result_t));
    res->stmt = stmt;
    res->binds = NULL;
    res->dataset = NULL;
    res->meta = NULL;
    res->num_of_dataset = 0;
    res->num_of_bind = num_of_bind;
    if (num_of_bind > 0) {
        res->binds = (MYSQL_BIND *)_cc_calloc(num_of_bind, sizeof(MYSQL_BIND));
    }
#if DEBUG_EXECUTION_TIME
    res->execution_time = execution_time;
#endif
    *result = res;
    return true;
}

_CC_API_PRIVATE(bool_t) _mysql_reset(_cc_sql_result_t *result) {
    if (result->stmt == NULL) {
        return false;
    }

    mysql_stmt_reset(result->stmt);
    bzero(result->binds, sizeof(MYSQL_BIND) * result->num_of_bind);
    return true;
}

_CC_API_PRIVATE(bool_t) _mysql_step(_cc_sql_result_t *result) {
    if (result->stmt == NULL) {
        return false;
    }

    if (result->binds) {
        if (mysql_stmt_bind_param(result->stmt, result->binds) != 0) {
            _cc_logger_error("mysql_stmt_bind_param error %d: %s", mysql_stmt_errno(result->stmt), mysql_stmt_error(result->stmt));
            return false;
        }
    }

    if (mysql_stmt_execute(result->stmt) != 0) {
        _cc_logger_error("mysql_stmt_execute error %d: %s", mysql_stmt_errno(result->stmt), mysql_stmt_error(result->stmt));
        return false;
    }

    result->step = true;
    return __dataset(result);
}

/**/
_CC_API_PRIVATE(bool_t) _mysql_next_result(_cc_sql_result_t *result) {
    _cc_assert(result != NULL);

    __free_dataset(result);

    if (!mysql_stmt_next_result(result->stmt)) {
        _cc_logger_error("mysql_stmt_next_result error %d: %s", mysql_stmt_errno(result->stmt), mysql_stmt_error(result->stmt));
        return false;
    }

    return __dataset(result);
}

_CC_API_PRIVATE(bool_t) _mysql_fetch(_cc_sql_result_t *result) {
    _cc_assert(result != NULL && result->stmt != NULL);
    if (result->step == false) {
        if (!_mysql_step(result)) {
            return false;
        }
    }

    return mysql_stmt_fetch(result->stmt) == 0;
}

_CC_API_PRIVATE(uint64_t) _mysql_get_num_rows(_cc_sql_result_t *result) {
    _cc_assert(result != NULL && result->stmt != NULL);
    return mysql_stmt_num_rows(result->stmt);
}

_CC_API_PRIVATE(int32_t) _mysql_get_num_fields(_cc_sql_result_t *result) {
    _cc_assert(result != NULL && result->stmt != NULL);
    return result->num_of_dataset;
}

_CC_API_PRIVATE(bool_t) _mysql_free_result(_cc_sql_result_t *result) {
    _cc_assert(result != NULL);
    __free_dataset(result);

    if (result->binds) {
        int i;
        for (i = 0; i < result->num_of_bind; i++) {
            MYSQL_BIND *data = &(result->binds[i]);
            if (data->buffer_type == MYSQL_TYPE_TIMESTAMP) {
                _cc_free(data->buffer);
            }
        }
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
_CC_API_PRIVATE(bool_t)
_mysql_bind(_cc_sql_result_t *result, int32_t index, const void *value, size_t length, uint8_t type) {
    MYSQL_BIND *b;
    _cc_assert(result != NULL);
    if (index >= result->num_of_bind) {
        return false;
    }

    b = &result->binds[index];
    b->buffer = (void *)value;
    b->buffer_length = (unsigned long)length;

    switch (type) {
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
            b->buffer_length = (unsigned long)_tcslen((tchar_t *)value);
        }
    } break;
    case _CC_SQL_TYPE_BLOB_:
        b->is_unsigned = false;
        b->buffer_type = MYSQL_TYPE_BLOB;
        break;
    case _CC_SQL_TYPE_NULL_:
        b->is_unsigned = false;
        b->buffer_type = MYSQL_TYPE_NULL;
        break;
    case _CC_SQL_TYPE_TIMESTAMP_:
    case _CC_SQL_TYPE_DATETIME_: {
        struct tm *timeinfo = (struct tm *)value;
        MYSQL_TIME *datetime;

        b->buffer_length = sizeof(MYSQL_TIME);
        b->buffer = _cc_malloc(b->buffer_length);
        b->buffer_type = MYSQL_TYPE_TIMESTAMP;
        b->is_unsigned = false;

        datetime = (MYSQL_TIME *)b->buffer;
        datetime->year = timeinfo->tm_year + 1900;
        datetime->month = timeinfo->tm_mon + 1;
        datetime->day = timeinfo->tm_mday;
        datetime->hour = timeinfo->tm_hour;
        datetime->minute = timeinfo->tm_min;
        datetime->second = timeinfo->tm_sec;
    } break;
    case _CC_SQL_TYPE_JSON_: {
        b->is_unsigned = false;
        b->buffer_type = MYSQL_TYPE_JSON;
        if (length == -1) {
            b->buffer_length = (unsigned long)_tcslen((tchar_t *)value);
        }
    } break;
    }
    return true;
}

/**/
_CC_API_PRIVATE(uint64_t) _mysql_get_last_id(_cc_sql_t *ctx, _cc_sql_result_t *result) {
    _cc_assert(result != NULL && result->stmt != NULL);
    return mysql_stmt_insert_id(result->stmt);
}

_CC_API_PRIVATE(pvoid_t) _mysql_get_stmt(_cc_sql_result_t *result) {
    return result->stmt;
}

/**/
_CC_API_PRIVATE(int32_t) _mysql_get_int(_cc_sql_result_t *result, int32_t index) {
    MYSQL_BIND *b;
    _cc_assert(result != NULL);
    if (index >= result->num_of_dataset) {
        return 0;
    }
    b = &result->dataset[index];
    return *(int32_t *)(b->buffer);
}

/**/
_CC_API_PRIVATE(int64_t) _mysql_get_int64(_cc_sql_result_t *result, int32_t index) {
    MYSQL_BIND *b;
    _cc_assert(result != NULL);
    if (index >= result->num_of_dataset) {
        return 0;
    }

    b = &result->dataset[index];
    return *(int64_t *)(b->buffer);
}

/**/
_CC_API_PRIVATE(float64_t) _mysql_get_float(_cc_sql_result_t *result, int32_t index) {
    MYSQL_BIND *b;
    _cc_assert(result != NULL);
    if (index >= result->num_of_dataset) {
        return false;
    }
    b = &result->dataset[index];
    return *(float64_t *)(b->buffer);
}

/**/
_CC_API_PRIVATE(size_t) _mysql_get_string(_cc_sql_result_t *result, int32_t index, tchar_t *buffer, size_t length) {
    MYSQL_BIND *b;
    size_t bytes_length;
    _cc_assert(result != NULL);
    if (index >= result->num_of_dataset) {
        return 0;
    }
    *buffer = 0;

    b = &result->dataset[index];
    if (((tchar_t *)b->buffer)[0] == 0) {
        return 0;
    }

    bytes_length = _tcslen((tchar_t *)b->buffer);
    if (bytes_length >= length) {
        bytes_length = length - 1;
    }

    //_tcsncpy(buffer, b->buffer, bytes_length);
    memcpy(buffer, b->buffer, bytes_length * sizeof(tchar_t));
    buffer[bytes_length] = 0;
    return bytes_length;
}

/**/
_CC_API_PRIVATE(size_t) _mysql_get_blob(_cc_sql_result_t *result, int32_t index, byte_t *buffer, size_t length) {
    MYSQL_BIND *b;
    _cc_assert(result != NULL);
    if (index >= result->num_of_dataset) {
        return 0;
    }

    b = &result->dataset[index];
    if (buffer && length > 0) {
        length = b->buffer_length > length ? length : b->buffer_length;
        memcpy(buffer, b->buffer, length);
        return length;
    }

    return b->buffer_length;
}

/**/
_CC_API_PRIVATE(bool_t) _mysql_get_datetime(_cc_sql_result_t *result, int32_t index, struct tm *timeinfo) {
    MYSQL_BIND *b;
    MYSQL_TIME *datetime;
    _cc_assert(result != NULL);
    if (index >= result->num_of_dataset) {
        return false;
    }
    b = &result->dataset[index];
    if (b->buffer_type == MYSQL_TYPE_DATETIME || b->buffer_type == MYSQL_TYPE_DATE ||
        b->buffer_type == MYSQL_TYPE_TIME) {
        datetime = (MYSQL_TIME *)(b->buffer);

        timeinfo->tm_year = 0;
        timeinfo->tm_mon = 0;
        timeinfo->tm_mday = 0;

        if (datetime->year >= 1900 && datetime->month > 0) {
            timeinfo->tm_year = datetime->year - 1900;
            timeinfo->tm_mon = datetime->month - 1;
            timeinfo->tm_mday = datetime->day;
            _cc_civil_to_days(datetime->year,datetime->month,datetime->day,&timeinfo->tm_wday,&timeinfo->tm_yday);
        }

        timeinfo->tm_hour = datetime->hour;
        timeinfo->tm_min = datetime->minute;
        timeinfo->tm_sec = datetime->second;
        timeinfo->tm_isdst = 0;

        return true;
    }
    return false;
}
/**/
_CC_API_PUBLIC(bool_t) _cc_register_mysql(_cc_sql_delegate_t *delegator) {
#define SET(x) delegator->x = _mysql_##x

    if (_cc_unlikely(delegator == NULL)) {
        return false;
    }

    /**/
    SET(connect);
    SET(disconnect);
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
