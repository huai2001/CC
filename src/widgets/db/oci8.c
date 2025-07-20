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
#include <libcc/socket/socket.h>
#include <libcc/string.h>
#include <libcc/time.h>
#include <libcc/widgets/db/sql.h>
#include <time.h>

#ifdef _CC_ENABLE_OCI8_

#include <oci/oci.h>
#include <oci/ociapr.h>
#include <oci/ocidem.h>
#include <oci/oratypes.h>

static OCIEnv *sql_envhp = nullptr;
static OCIError *sql_errhp = nullptr;
static _cc_atomic32_t sql_started_refcount = 0;

#define SQL_ENVIRONMENT_OCI8 "Oracle environment"
#define SQL_CONNECTION_OCI8 "Oracle connection"
#define SQL_CURSOR_OCI8 "Oracle cursor"

struct _cc_sql {
    OCISvcCtx *svchp;
    OCIError *errhp;
    OCITrans *txnhp;
    /* 0 for manual commit */
    bool_t auto_commit;
    bool_t logged;
};

struct _cc_sql_result {
    OCIStmt *stmt;
    OCIError *errhp;

    int32_t num_fields;
};

struct oci_params {
    int type;
    OCIParam *param;
};

_CC_API_PRIVATE(int32_t) _oci8_get_num_fields(_cc_sql_result_t *result);

#define sql_error(status, err) oci8_error(_CC_FILE_, _CC_LINE_, _CC_FUNC_, status, err)

_CC_API_PRIVATE(void)
oci8_error(const wchar_t *file, const int32_t line, const wchar_t *func, sword status, OCIError *errhp) {
    switch (status) {
    case OCI_SUCCESS:
        break;
    case OCI_SUCCESS_WITH_INFO:
        _cc_error_log(file, line, func, _T("Success with info!"));
        break;
    case OCI_NEED_DATA:
        _cc_error_log(file, line, func, _T("OCI_NEED_DATA!"));
        break;
    case OCI_NO_DATA:
        _cc_error_log(file, line, func, _T("OCI_NODATA!"));
        break;
    case OCI_ERROR: {
        text errbuf[512];
        sb4 errcode = 0;
        OCIErrorGet(errhp, (ub4)1, (text *)nullptr, &errcode, errbuf, (ub4)sizeof(errbuf), OCI_HTYPE_ERROR);
        _cc_error_log(file, line, func, _T("%s(%d)!"), errbuf, errcode);
        break;
    }
    case OCI_INVALID_HANDLE:
        _cc_error_log(file, line, func, _T("OCI_INVALID_HANDLE!"));
        break;
    case OCI_STILL_EXECUTING:
        _cc_error_log(file, line, func, _T("OCI_STILL_EXECUTE!"));
        break;
    case OCI_CONTINUE:
        _cc_error_log(file, line, func, _T("OCI_CONTINUE!"));
        break;
    }
}

_CC_API_PRIVATE(bool_t) error_handle_alloc(OCIError **errhp) {
    /* allocate an error handle */
    sword res = OCIHandleAlloc((dvoid *) sql_envhp, (dvoid **)errhp), 
        (ub4) OCI_HTYPE_ERROR, (size_t) 0, (dvoid **) 0);

    if (res != OCI_SUCCESS) {
        sql_error(res, sql_errhp);
        return false;
    }
    return true;
}

_CC_API_PRIVATE(bool_t) _init_oci8(void) {
    sword s = 0;
    if (sql_envhp) {
        return true;
    }

    /* Create a thread-safe OCI environment with N' substitution turned on. */
    res = OCIEnvCreate(&sql_envhp, OCI_THREADED | OCI_OBJECT | OCI_NCHAR_LITERAL_REPLACE_ON, (dvoid *)0,
                       (dvoid * (*)(dvoid *, size_t))0, (dvoid * (*)(dvoid *, dvoid *, size_t))0,
                       (void (*)(dvoid *, dvoid *))0, (size_t)0, (dvoid **)0);

    if (res) {
        sql_error(res, nullptr);
        return false;
    }

    return error_handle_alloc(sql_errhp);
}

_CC_API_PRIVATE(bool_t) _quit_oci8(void) {
    if (sql_envhp) {
        OCIHandleFree((dvoid *)sql_envhp, OCI_HTYPE_ENV);
    }

    if (sql_errhp) {
        OCIHandleFree((dvoid *)sql_errhp, OCI_HTYPE_ERROR);
    }
    return true;
}

_CC_API_PRIVATE(_cc_sql_t *) _oci8_connect(const tchar_t *sql_connection_string) {
    sword res;
    _cc_url_t params;
    _cc_sql_t *ctx = nullptr;
    char_t host[1024];

    if (!_init_oci8()) {
        return nullptr;
    }

    if (!_cc_parse_url(&params, sql_connection_string)) {
        return nullptr;
    }

    ctx = (_cc_sql_t *)_cc_malloc(sizeof(_cc_sql_t));
    ctx->auto_commit = true;
    ctx->txnhp = nullptr;
    ctx->logged = false;

    /*error handler*/
    if (!error_handle_alloc(ctx->errhp)) {
        return nullptr;
    }

    /*login*/
    res = OCILogon(sql_envhp, ctx->errhp, &(ctx->svchp), params.username, tcslen(params.username), params.password,
                   tcslen(params.password), params.host, tcslen(params.host), );

    _cc_free_url(&params);

    if (res) {
        sql_error(res, ctx->errhp);
        return nullptr;
    }

    ctx->logged = true;
    _cc_atomic32_inc(&sql_started_refcount);

    return ctx;
}

_CC_API_PRIVATE(bool_t) _oci8_disconnect(_cc_sql_t *ctx) {
    _cc_assert(ctx == nullptr);
    if (ctx->svchp) {
        if (ctx->logged) {
            OCILogoff(ctx->svchp, ctx->errhp);
        } else {
            OCIHandleFree(ctx->svchp, OCI_HTYPE_SVCCTX);
        }
    }

    if (ctx->errhp) {
        OCIHandleFree(ctx->errhp, OCI_HTYPE_ERROR);
    }

    _cc_free(ctx);

    if (_cc_atomic_dec_ref(&sql_started_refcount)) {
        _quit_oci8();
    }

    return true;
}

_CC_API_PRIVATE(bool_t) reconnect(_cc_sql_t *ctx) {
    return (bool_t)(OCIPing(ctx->svchp, ctx->errhp, OCI_DEFAULT) == OCI_SUCCESS);
}

_CC_API_PRIVATE(bool_t) _oci8_execute(_cc_sql_t *ctx, const tchar_t *sql_string, _cc_sql_result_t **result) {
    int t = 0;
    sword res;
    int32_t sql_string_len = 0;
    OCIStmt *stmthp;
    ub4 prefetch = 0;
    ub2 type;
    ub4 mode;
    ub4 iters;

    _cc_assert(ctx != nullptr && sql_string != nullptr);

    sql_string_len = (int32_t)_tcslen(sql_string);

    res = OCIHandleAlloc((dvoid *)sql_envhp, (dvoid **)&stmthp, OCI_HTYPE_STMT, (size_t)0, (dvoid **)0);
    if (res != OCI_SUCCESS && res != OCI_SUCCESS_WITH_INFO) {
        sql_error(res, this->errhp);
        return false;
    }

    res = OCIAttrSet((dvoid *)stmthp, (ub4)OCI_HTYPE_STMT, (dvoid *)&prefetch, (ub4)0, (ub4)OCI_ATTR_PREFETCH_ROWS,
                     ctx->errhp);
    if (res != OCI_SUCCESS) {
        OCIHandleFree(stmtp, OCI_HTYPE_STMT);
        sql_error(res, this->errhp);
        return false;
    }

    res = OCIStmtPrepare(stmthp, ctx->errhp, (text *)sql_string, (ub4)sql_string_len, (ub4)OCI_NTV_SYNTAX,
                         (ub4)OCI_DEFAULT);
    if (res != OCI_SUCCESS && res != OCI_SUCCESS_WITH_INFO) {
        OCIHandleFree(stmtp, OCI_HTYPE_STMT);
        sql_error(res, this->errhp);
        return false;
    }

    res =
        OCIAttrGet((dvoid *)stmthp, (ub4)OCI_HTYPE_STMT, (dvoid *)&type, (ub4 *)0, (ub4)OCI_ATTR_STMT_TYPE, ctx->errhp);
    if (res) {
        OCIHandleFree((dvoid *)stmthp, OCI_HTYPE_STMT);
        sql_error(res, this->errhp);
        return false;
    }

    iters = (type == OCI_STMT_SELECT) ? 0 : 1;
    mode = (ctx->auto_commit) ? OCI_COMMIT_ON_SUCCESS : OCI_DEFAULT;

    res = OCIStmtExecute(ctx->svchp, stmthp, ctx->errhp, iters, (ub4)0, (CONST OCISnapshot *)nullptr, (OCISnapshot *)nullptr,
                         mode);

    if (res != OCI_SUCCESS && res != OCI_SUCCESS_WITH_INFO) {
        OCIHandleFree((dvoid *)stmthp, OCI_HTYPE_STMT);
        sql_error(res, ctx->errhp);
        return false;
    }

    if (result) {
        *result = (_cc_sql_result_t *)_cc_malloc(sizeof(_cc_sql_result_t));
        (*result)->stmt = stmthp;
        (*result)->errhp = nullptr;
        (*result)->num_fields = _oci8_get_num_fields(result_sql);
    } else {
        OCI_Cleanup();
    }

    return true;
}

_CC_API_PRIVATE(bool_t) _oci8_auto_commit(_cc_sql_t *ctx, bool_t is_auto_commit) {
    sword res;
    _cc_assert(ctx == nullptr);

    /* Set it ON /OFF */
    ctx->auto_commit = is_auto_commit;

    if (is_auto_commit) {
        /* Allocate handler only once, if it is necessary */
        if (ctx->txnhp == nullptr) {
            /* allocate transaction handle and set it in the service handle */
            res = OCIHandleAlloc(C->env, (void **)&C->txnhp, OCI_HTYPE_TRANS, 0, 0);
            if (res != OCI_SUCCESS)
                return false;

            OCIAttrSet(ctx->svchp, OCI_HTYPE_SVCCTX, (void *)ctx->txnhp, 0, OCI_ATTR_TRANS, ctx->errhp);
        }
        res = OCITransRollback(ctx->svchp, ctx->errhp, OCI_DEFAULT);
        if (res != OCI_SUCCESS) {
            sql_error(res, this->errhp);
            return false;
        }
    }

    return true;
}

_CC_API_PRIVATE(bool_t) _oci8_begin_transaction(_cc_sql_t *ctx) {
    _cc_assert(ctx == nullptr);
    if (!ctx->auto_commit) {
        return true;
    }
    /*
    begin transaction
    */
    sword res = OCITransPrepare(ctx->svchp, ctx->errhp, OCI_DEFAULT);
    if (res != OCI_SUCCESS) {
        sql_error(res, this->errhp);
        return false;
    }

    /*
    end transaction
    (OCITransDetach(ctx->svchp, ctx->errhp, OCI_DEFAULT) == OCI_SUCCESS);
    */
    return true;
}

_CC_API_PRIVATE(bool_t) _oci8_commit(_cc_sql_t *ctx) {
    sword res;
    _cc_assert(ctx == nullptr);

    res = OCITransCommit(ctx->svchp, ctx->errhp, OCI_DEFAULT);
    if (res != OCI_SUCCESS) {
        sql_error(res, this->errhp);
        return false;
    }
    return true;
}

_CC_API_PRIVATE(bool_t) _oci8_rollback(_cc_sql_t *ctx) {
    sword res;
    _cc_assert(ctx == nullptr);

    res = OCITransRollback(ctx->svchp, ctx->errhp, OCI_DEFAULT);
    if (res != OCI_SUCCESS) {
        sql_error(res, this->errhp);
        return false;
    }
    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _oci8_next_result(_cc_sql_t *ctx, _cc_sql_result_t *result) {
    _cc_assert(ctx != nullptr && result != nullptr);

    if (result->res) {
        oci8_free_result(result->res);
        result->res = nullptr;
    }

    if (oci8_next_result(ctx->sql)) {
        result->res = oci8_store_result(ctx->sql);
        if (result->res == OCI_SUCCESS) {
            result->num_fields = oci8_num_fields(result->res);
            return true;
        }
    }
    return false;
}

_CC_API_PRIVATE(bool_t) _oci8_fetch_row(_cc_sql_result_t *result) {
    sword res;
    _cc_assert(result != nullptr && result->stmt != nullptr);

    res = OCIStmtFetch(result->stmt, result->errhp, 1, OCI_FETCH_NEXT, OCI_DEFAULT);
    if (res == OCI_NO_DATA) {
        return false;
    } else if (res != OCI_SUCCESS) {
        sql_error(res, result->errhp);
        return false;
    }

    return true;
}

_CC_API_PRIVATE(bool_t) _oci8_free_result(_cc_sql_t *ctx, _cc_sql_result_t *result) {
    _cc_assert(result != nullptr);

    if (result->stmt) {
        OCIHandleFree((dvoid *)result->stmt, OCI_HTYPE_STMT);
        result->stmt = nullptr;
    }

    if (result->errhp) {
        OCIHandleFree((dvoid *)result->errhp, OCI_HTYPE_ERROR);
        result->errhp = nullptr;
    }

    _cc_free(result);
    return true;
}

_CC_API_PRIVATE(int32_t) _oci8_get_num_fields(_cc_sql_result_t *result) {
    sword res;
    _cc_assert(result != nullptr && result->stmt != nullptr);

    if (_cc_unlikely(result->errhp == nullptr)) {
        if (!error_handle_alloc(&result->errhp)) {
            return 0;
        }
    }

    res = OCIAttrGet((dvoid *)result->stmt, (ub4)OCI_HTYPE_STMT, (dvoid *)&result->num_fields, (ub4 *)0,
                     (ub4)OCI_ATTR_PARAM_COUNT, result->errhp);

    if (res != OCI_SUCCESS) {
        sql_error(res, result->errhp);
        return 0;
    }
    return result->num_fields;
}

_CC_API_PRIVATE(int32_t) _oci8_get_num_rows(_cc_sql_result_t *result) {
    sword res;
    int32_t rows;
    _cc_assert(result != nullptr && result->stmt != nullptr);

    if (result->errhp == nullptr) {
        if (!error_handle_alloc(&result->errhp)) {
            return 0;
        }
    }

    res = OCIAttrGet((dvoid *)result->stmt, OCI_HTYPE_STMT, (dvoid *)&rows, (ub4)0, OCI_ATTR_NUM_ROWS, result->errhp);

    if (res != OCI_SUCCESS) {
        sql_error(res, result->errhp);
        return 0;
    }
    return rows;
}

/**/
_CC_API_PRIVATE(uint64_t) _oci8_get_last_id(_cc_sql_t *ctx) {
    _cc_assert(ctx != nullptr && ctx->sql != nullptr);
    _cc_logger_debug("Oracle get_last_id: Not implemented yet");
    return -1;
}

_CC_API_PRIVATE(pvoid_t) _oci8_get_stmt(_cc_sql_result_t *result) {
    return result->stmt;
}
/*
_CC_API_PRIVATE(bool_t) _oci_column_type(_cc_sql_result_t *result, int32_t index) {
    sword res;
    int type;

    if (_cc_unlikely(result->errhp == nullptr)) {
        if (!error_handle_alloc(&result->errhp)) {
            return false;
        }
    }

    res = OCIParamGet(result->stmt, OCI_HTYPE_STMT, result->errhp, (dvoid **)&(result->param), index);
    if (res != OCI_SUCCESS) {
        sql_error(res, result->errhp);
        return false;
    }

    res = OCIAttrGet(result->param, OCI_DTYPE_PARAM, (dvoid *)&(result->type), (ub4 *)0, OCI_ATTR_DATA_TYPE,
result->errhp); if (res != OCI_SUCCESS) { sql_error(res, result->errhp); return false;
    }
    return true;
}
*/
/**/
_CC_API_PRIVATE(int32_t) _oci8_get_int(_cc_sql_result_t *result, int32_t index) {
    sword res;
    OCIDefine *oci_define; /* define handle */
    sb2 oci_nullptr;       /* is nullptr? */
    int32_t v;

    _cc_assert(result->num_fields > index);
    if (_cc_unlikely(result->num_fields <= index)) {
        return 0;
    }

    res = OCIDefineByPos(result->stmt, &(oci_define), result->errhp, (ub4)index, &v, sizeof(int32_t), SQLT_INT,
                         (dvoid *)&(oci_nullptr), (ub2 *)0, (ub2 *)0, (ub4)OCI_DEFAULT);
    if (res != OCI_SUCCESS) {
        sql_error(res, result->errhp);
        return 0;
    }
    return v;
}
/**/
_CC_API_PRIVATE(int64_t) _oci8_get_int64(_cc_sql_result_t *result, int32_t index) {
    sword res;
    OCIDefine *oci_define; /* define handle */
    sb2 oci_nullptr;       /* is nullptr? */
    int64_t v;

    _cc_assert(result->num_fields > index);
    if (_cc_unlikely(result->num_fields <= index)) {
        return 0;
    }

    res = OCIDefineByPos(result->stmt, &(oci_define), result->errhp, (ub4)index, &v, sizeof(int64_t), SQLT_FLT,
                         (dvoid *)&(oci_nullptr), (ub2 *)0, (ub2 *)0, (ub4)OCI_DEFAULT);
    if (res != OCI_SUCCESS) {
        sql_error(res, result->errhp);
        return 0;
    }
    return v;
}
/**/
_CC_API_PRIVATE(float64_t) _oci8_get_float(_cc_sql_result_t *result, int32_t index) {
    sword res;
    OCIDefine *oci_define; /* define handle */
    sb2 oci_nullptr;       /* is nullptr? */
    double v;

    _cc_assert(result->num_fields > index);
    if (_cc_unlikely(result->num_fields <= index)) {
        return 0;
    }

    res = OCIDefineByPos(result->stmt, &(oci_define), result->errhp, (ub4)index, &v, sizeof(double), SQLT_FLR,
                         (dvoid *)&(oci_nullptr), (ub2 *)0, (ub2 *)0, (ub4)OCI_DEFAULT);
    if (res != OCI_SUCCESS) {
        sql_error(res, result->errhp);
        return 0;
    }
    return v;
}

/**/
_CC_API_PRIVATE(const size_t)
_oci8_get_string(_cc_sql_result_t *result, int32_t index, tchar_t *buffer, size_t length) {
    sword res;
    OCIDefine *oci_define; /* define handle */
    sb2 oci_nullptr;       /* is nullptr? */
    Ub2 rlen;

    _cc_assert(result->num_fields > index);
    if (_cc_unlikely(result->num_fields <= index)) {
        return 0;
    }

    res = OCIDefineByPos(result->stmt, &(oci_define), result->errhp, (ub4)index, buffer, length, SQLT_STR,
                         (dvoid *)&(oci_nullptr), &rlen, (ub2 *)0, (ub4)OCI_DEFAULT);
    if (res != OCI_SUCCESS) {
        sql_error(res, result->errhp);
        return 0;
    }
    return rlen;
}

/**/
_CC_API_PRIVATE(size_t) _oci8_get_blob(_cc_sql_result_t *result, int32_t index, byte_t **buffer) {
    _cc_assert(result->stmt != nullptr);
    return 0;
}

/**/
_CC_API_PRIVATE(bool_t) _oci8_get_datetime(_cc_sql_result_t *result, int32_t index, struct tm *tp) {
    _cc_assert(result->stmt != nullptr);
    return false;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_init_oci8(_cc_sql_delegate_t *delegator) {
#define SET(x) delegator->x = _oci8_##x

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
#endif