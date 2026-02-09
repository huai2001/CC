#ifndef _C_CC_JSON_C_H_INCLUDED_
#define _C_CC_JSON_C_H_INCLUDED_

#include <libcc/json.h>
#include <libcc/alloc.h>
#include "../misc/misc.c.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Limits how deeply nested arrays/objects can be before JSON rejects to parse
 * them. This is to prevent stack overflows. */
#define _JSON_NESTING_LIMIT_    1000

#define _JSON_ARRAY_START_      _T('[')
#define _JSON_ARRAY_END_        _T(']')

#define _JSON_OBJECT_START_     _T('{')
#define _JSON_OBJECT_END_       _T('}')

#define _JSON_NEXT_TOKEN_       _T(',')
#define _JSON_OBJECT_TOKEN_     _T(':')

void _json_free_object_rb_node(_cc_rb_t *node);
void _json_free_node(_cc_json_t *item);

void _json_array_alloc(_cc_json_t* ctx, size_t size);

size_t _json_array_push(_cc_json_t *ctx, _cc_json_t *item);
bool_t _json_object_push(_cc_json_t *ctx, _cc_json_t *item, bool_t replacement);

void _free_json_array(_cc_json_t*);
void _free_json_object(_cc_json_t*);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_JSON_C_H_INCLUDED_*/
