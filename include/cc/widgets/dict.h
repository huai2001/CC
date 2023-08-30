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
#ifndef _C_CC_DICTIONARY_H_INCLUDED_
#define _C_CC_DICTIONARY_H_INCLUDED_

#include <cc/alloc.h>
#include <cc/buf.h>
#include <cc/rbtree.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef _cc_rbtree_t _cc_dict_t;

typedef struct _cc_dict_node {
    tchar_t *name;
    tchar_t *value;
    _cc_rbtree_iterator_t node;
} _cc_dict_node_t;

/**/
_cc_buf_t *_cc_dict_join(_cc_dict_t *ctx, const tchar_t isquoted);
/**/
bool_t _cc_dict_split(_cc_dict_t *ctx, const tchar_t *str, const tchar_t isquoted);
/**/
bool_t _cc_dict_insert(_cc_dict_t *ctx, tchar_t *name, tchar_t *value);
/**/
const tchar_t *_cc_dict_find(_cc_dict_t *ctx, const tchar_t *name);
/**/
bool_t _cc_dict_free(_cc_dict_t *ctx);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_DICTIONARY_H_INCLUDED_*/
