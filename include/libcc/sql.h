#ifndef _C_CC_SQL_H_INCLUDED_
#define _C_CC_SQL_H_INCLUDED_

#include "types.h"

#ifndef __CC_WINDOWS__
#define _CC_USE_SYSTEM_SQLITE3_LIB_     1
#endif

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _cc_sql_delegate _cc_sql_delegate_t;
typedef struct _cc_sql _cc_sql_t;
typedef struct _cc_sql_result _cc_sql_result_t;

#define _CC_SQL(X) X, (size_t)(sizeof(X) - 1)

enum {
    _CC_SQL_TYPE_NULL_ = 0,
    _CC_SQL_TYPE_INT8_,
    _CC_SQL_TYPE_INT16_,
    _CC_SQL_TYPE_INT32_,
    _CC_SQL_TYPE_INT64_,
    _CC_SQL_TYPE_UINT8_,
    _CC_SQL_TYPE_UINT16_,
    _CC_SQL_TYPE_UINT32_,
    _CC_SQL_TYPE_UINT64_,
    _CC_SQL_TYPE_FLOAT_,
    _CC_SQL_TYPE_DOUBLE_,
    _CC_SQL_TYPE_STRING_,
    _CC_SQL_TYPE_BLOB_,
    _CC_SQL_TYPE_DATETIME_,
    _CC_SQL_TYPE_TIMESTAMP_,
    _CC_SQL_TYPE_JSON_
};

struct _cc_sql_delegate {
    /**/
    _cc_sql_t *(*connect)(const tchar_t *);
    /**/
    bool_t (*disconnect)(_cc_sql_t *);
    /**/
    bool_t (*execute)(_cc_sql_t *, const tchar_t *, size_t, _cc_sql_result_t **);
    /**/
    bool_t (*auto_commit)(_cc_sql_t *, bool_t);
    /**/
    bool_t (*begin_transaction)(_cc_sql_t *);
    /**/
    bool_t (*commit)(_cc_sql_t *);
    /**/
    bool_t (*rollback)(_cc_sql_t *);
    /**/
    bool_t (*reset)(_cc_sql_result_t *);
    /**/
    bool_t (*step)(_cc_sql_result_t *);
    /**/
    bool_t (*next_result)(_cc_sql_result_t *);
    /**/
    bool_t (*free_result)(_cc_sql_result_t *);
    /**/
    int32_t (*get_num_fields)(_cc_sql_result_t *);
    /**/
    uint64_t (*get_num_rows)(_cc_sql_result_t *);
    /**/
    bool_t (*fetch)(_cc_sql_result_t *);
    /**/
    pvoid_t (*get_stmt)(_cc_sql_result_t *);
    /**/
    uint64_t (*get_last_id)(_cc_sql_t *, _cc_sql_result_t *);
    /**/
    bool_t (*bind)(_cc_sql_result_t *, int32_t, const void *, size_t, uint8_t);
    /**/
    int32_t (*get_int)(_cc_sql_result_t *, int32_t);
    /**/
    int64_t (*get_int64)(_cc_sql_result_t *, int32_t);
    /**/
    float64_t (*get_float)(_cc_sql_result_t *, int32_t);
    /**/
    size_t (*get_string)(_cc_sql_result_t *, int32_t, tchar_t*, size_t);
    /**/
    size_t (*get_blob)(_cc_sql_result_t *, int32_t, byte_t **);
    /**/
    bool_t (*get_datetime)(_cc_sql_result_t *, int32_t, struct tm*);
};

/**/
_CC_API_PUBLIC(bool_t) _cc_register_mysql(_cc_sql_delegate_t *delegator);
/**/
_CC_API_PUBLIC(bool_t) _cc_register_sqlsvr(_cc_sql_delegate_t *delegator);
/**/
_CC_API_PUBLIC(bool_t) _cc_register_sqlite(_cc_sql_delegate_t *delegator);
/**/
_CC_API_PUBLIC(bool_t) _cc_register_oci8(_cc_sql_delegate_t *delegator);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _C_CC_SQL_H_INCLUDED_*/
