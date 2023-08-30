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
#ifndef _C_CC_SQL_H_INCLUDED_
#define _C_CC_SQL_H_INCLUDED_

#include "../time.h"
#include "../url.h"
#include "../buf.h"

#if defined(_CC_DB_EXPORT_SHARED_LIBRARY_)
    #define _CC_DB_API(t) _CC_API_EXPORT_ t
#elif defined(_CC_DB_IMPORT_SHARED_LIBRARY_)
    #define _CC_DB_API(t) _CC_API_IMPORT_ t
#else
    #define _CC_DB_API(t) t
#endif

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _cc_sql_driver _cc_sql_driver_t;
typedef struct _cc_sql _cc_sql_t;
typedef struct _cc_sql_result _cc_sql_result_t;


typedef enum _sql_enum_field_types {
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
}_sql_enum_field_types_t;

struct _cc_sql_driver {
    /**/
    _cc_sql_t *(*connect)(const tchar_t *);
    /**/
    bool_t (*disconnect)(_cc_sql_t *);
    /**/
    bool_t (*prepare)(_cc_sql_t *, const tchar_t *, _cc_sql_result_t **);
    /**/
    bool_t (*reset)(_cc_sql_t *, _cc_sql_result_t *);
    /**/
    bool_t (*step)(_cc_sql_t *, _cc_sql_result_t *);
    /**/
    bool_t (*execute)(_cc_sql_t *, const tchar_t *, _cc_sql_result_t **);
    /**/
    bool_t (*auto_commit)(_cc_sql_t *, bool_t);
    /**/
    bool_t (*begin_transaction)(_cc_sql_t *);
    /**/
    bool_t (*commit)(_cc_sql_t *);
    /**/
    bool_t (*rollback)(_cc_sql_t *);
    /**/
    bool_t (*next_result)(_cc_sql_t *, _cc_sql_result_t *);
    /**/
    bool_t (*free_result)(_cc_sql_t *, _cc_sql_result_t *);
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
    bool_t (*bind)(_cc_sql_result_t *, int32_t, const void *, size_t, _sql_enum_field_types_t);

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
    struct tm* (*get_datetime)(_cc_sql_result_t *, int32_t);
};

/**
 * @brief Initialize mysql _cc_sql_driver structure
 *
 * @param driver _cc_sql_driver structure
 *
 * @return true if successful or false on error.
 */
_CC_DB_API(bool_t) _cc_init_mysql(_cc_sql_driver_t *driver);
/**
 * @brief Initialize SQLServer _cc_sql_driver structure
 *
 * @param driver _cc_sql_driver structure
 *
 * @return true if successful or false on error.
 */
_CC_DB_API(bool_t) _cc_init_sqlsvr(_cc_sql_driver_t *driver);
/**
 * @brief Initialize SQLite3 _cc_sql_driver structure
 *
 * @param driver _cc_sql_driver structure
 *
 * @return true if successful or false on error.
 */
_CC_DB_API(bool_t) _cc_init_sqlite(_cc_sql_driver_t *driver);
/**
 * @brief Initialize Oracle _cc_sql_driver structure
 *
 * @param driver _cc_sql_driver structure
 *
 * @return true if successful or false on error.
 */
_CC_DB_API(bool_t) _cc_init_oci8(_cc_sql_driver_t *driver);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _C_CC_SQL_H_INCLUDED_*/
