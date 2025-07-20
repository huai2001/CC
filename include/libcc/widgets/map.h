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
#ifndef _C_CC_WIDGETS_MAP_H_INCLUDED_
#define _C_CC_WIDGETS_MAP_H_INCLUDED_

#include "dylib.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef _cc_rbtree_t _cc_map_t;
/*
 * MAP Types:
 */
enum _CC_MAP_TYPES_ {
    _CC_MAP_NULL_ = 0,
    _CC_MAP_BOOLEAN_,
    _CC_MAP_FLOAT_,
    _CC_MAP_INT_,
    _CC_MAP_STRING_
};

typedef struct _cc_map_element {
    byte_t type;
    tchar_t *name;
    union {
        bool_t uni_boolean;
        int64_t uni_int;
        float64_t uni_float;
        tchar_t *uni_string;
    } element;
    _cc_rbtree_iterator_t lnk;
} _cc_map_element_t;

/**/
_CC_WIDGETS_API(_cc_map_element_t*) _cc_map_element_alloc(byte_t type);
/**/
_CC_WIDGETS_API(void) _cc_map_element_free(_cc_map_element_t *m);
/**/
_CC_WIDGETS_API(_cc_buf_t*) _cc_map_join(_cc_map_t *ctx, const tchar_t isquoted);
/**/
_CC_WIDGETS_API(bool_t) _cc_map_split(_cc_map_t *ctx, const tchar_t *str, const tchar_t isquoted);
/**/
_CC_WIDGETS_API(bool_t) _cc_map_push(_cc_map_t *ctx, _cc_map_element_t *data);
/**/
_CC_WIDGETS_API(const _cc_map_element_t*) _cc_map_find(_cc_map_t *ctx, const tchar_t *name);
/**/
_CC_WIDGETS_API(void) _cc_map_free(_cc_map_t *ctx);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_WIDGETS_MAP_H_INCLUDED_*/
