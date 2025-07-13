/*
 * Copyright libcc.cn@gmail.com. and other libCC contributors.
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
#ifndef _C_CC_JSON_C_H_INCLUDED_
#define _C_CC_JSON_C_H_INCLUDED_

#include <libcc/alloc.h>
#include <libcc/buf.h>
#include <libcc/string.h>
#include <libcc/widgets/json/json.h>
#include <math.h>
#include <wchar.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Limits how deeply nested arrays/objects can be before JSON rejects to parse
 * them. This is to prevent stack overflows. */
#define _JSON_NESTING_LIMIT_ 1000

#define _JSON_ARRAY_START_ _T('[')
#define _JSON_ARRAY_END_ _T(']')

#define _JSON_OBJECT_START_ _T('{')
#define _JSON_OBJECT_END_ _T('}')

#define _JSON_NEXT_TOKEN_ _T(',')
int32_t _json_push_object(_cc_rbtree_iterator_t *left, _cc_rbtree_iterator_t *right);
int32_t _json_get_object(_cc_rbtree_iterator_t*, pvoid_t);

void _json_free_object_rb_node(_cc_rbtree_iterator_t *node);
void _json_free_node(_cc_json_t *item);



void _json_array_alloc(_cc_json_t* ctx, size_t size);
bool_t _json_array_realloc(_cc_json_t *ctx, size_t size);
size_t _json_array_push(_cc_json_t *ctx, _cc_json_t *data);

void _destroy_json_array(_cc_json_t*);
void _destroy_json_object(_cc_json_t*);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_JSON_C_H_INCLUDED_*/
