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
#ifndef _C_CC_GENERIC_C_H_INCLUDED_
#define _C_CC_GENERIC_C_H_INCLUDED_

#include <libcc/alloc.h>
#include <libcc/buf.h>
#include <libcc/string.h>
#include <math.h>
#include <wchar.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const tchar_t* content;
    size_t position;
} _cc_syntax_error_t;

/**/
_CC_FORCE_INLINE_ bool_t _buf_char_put(_cc_buf_t *ctx, const tchar_t data) {
    return _cc_buf_append(ctx, (pvoid_t)&data, sizeof(tchar_t));
}

_CC_API_PUBLIC(tchar_t *) _convert_text(tchar_t *alloc_bytes, size_t alloc_length, const tchar_t *src, const tchar_t *endpos);
_CC_API_PUBLIC(void) _cc_syntax_error(_cc_syntax_error_t *error);
_CC_API_PUBLIC(const tchar_t*) _cc_get_syntax_error(void);
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_GENERIC_C_H_INCLUDED_*/
