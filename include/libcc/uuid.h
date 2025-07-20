

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
#ifndef _C_CC_UUID_H_INCLUDED_
#define _C_CC_UUID_H_INCLUDED_

#include "core.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**/
typedef byte_t _cc_uuid_t[16];

/**/
_CC_API_PUBLIC(void) _cc_uuid(_cc_uuid_t *u);
/**/
_CC_API_PUBLIC(void) _cc_uuid_to_bytes(_cc_uuid_t *u, const tchar_t *buf);

/**/
_CC_API_PUBLIC(int32_t) _cc_uuid_lower(_cc_uuid_t *u, tchar_t *buf, int32_t length);
/**/
_CC_API_PUBLIC(int32_t) _cc_uuid_upper(_cc_uuid_t *u, tchar_t *buf, int32_t length);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#endif /*_C_CC_UUID_H_INCLUDED_*/
