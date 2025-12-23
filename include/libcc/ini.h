#ifndef _C_CC_INI_H_INCLUDED_
#define _C_CC_INI_H_INCLUDED_

#include "sds.h"
#include "buf.h"
#include "rbtree.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/*
 * INI Types:
 */
enum _CC_INI_TYPES_ {
    _CC_INI_NULL_ = 0,
    _CC_INI_SECTION_,
    _CC_INI_BOOLEAN_,
    _CC_INI_FLOAT_,
    _CC_INI_INT_,
    _CC_INI_STRING_
};

typedef struct _cc_ini {
    byte_t type;    
    /**/
    _cc_sds_t name;

    union {
        bool_t uni_boolean;
        int64_t uni_int;
        float64_t uni_float;
        _cc_sds_t uni_string;
        _cc_rbtree_t uni_object;
    } element;
    _cc_rbtree_iterator_t lnk;
} _cc_ini_t;

/**/
_CC_API_PUBLIC(_cc_ini_t*) _cc_parse_ini(const tchar_t* src, size_t length);
/**/
_CC_API_PUBLIC(_cc_ini_t*) _cc_ini_from_file(const tchar_t* file_name);
/**/
_CC_API_PUBLIC(_cc_ini_t*) _cc_ini_parser(_cc_sbuf_t* const buffer);
/**/
_CC_API_PUBLIC(_cc_ini_t*) _cc_ini_find(_cc_ini_t* ctx, const tchar_t* name);
/**/
_CC_API_PUBLIC(_cc_sds_t) _cc_ini_find_string(_cc_ini_t* ctx, const tchar_t* name);
/**/
_CC_API_PUBLIC(void) _cc_free_ini(_cc_ini_t* ctx);
/**/
_CC_API_PUBLIC(const tchar_t*) _cc_ini_error(void);
/**/
_CC_API_PUBLIC(void) _cc_dump_ini(const _cc_ini_t* ctx, _cc_buf_t* buf);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_INI_H_INCLUDED_*/
