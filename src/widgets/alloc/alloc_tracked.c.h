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
#ifndef _C_CC_MEMORY_TRACKED_C_H_INCLUDED_
#define _C_CC_MEMORY_TRACKED_C_H_INCLUDED_

#include <cc/types.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#ifdef _CC_ENABLE_MEMORY_TRACKED_

#define MAX_TRACKED_BLOCKS 1024

#define _CC_MEM_MALLOC 0x01
#define _CC_MEM_CALLOC 0x02
#define _CC_MEM_REALLOC 0x03
#define _CC_MEM_FREE 0x04

#define _CC_MEM_CALLOC_FAIL 0x05
#define _CC_MEM_MALLOC_FAIL 0x06
#define _CC_MEM_REALLOC_FAIL 0x07
#define _CC_MEM_FREE_FAIL 0x08

typedef struct _ccmem_element _ccmem_element_t;
typedef struct _ccmem_element_link {
    _ccmem_element_t* element;
    _cc_list_iterator_t lnk;
} _ccmem_element_link_t;

bool_t _ccmem_tracked_empty(void);

/*
 * Remove an element with that key from the map
 */
pvoid_t _ccmem_tracked_remove(const pvoid_t);
/*
 * Get your pointer out of the hashmap with a key
 */
pvoid_t _ccmem_tracked_find(const pvoid_t);
/*
 * Add a pointer to the hashmap with some key
 */
bool_t _ccmem_tracked_insert(const pvoid_t, size_t, const tchar_t *, const tchar_t *, const int32_t, byte_t);

#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_MEMORY_TRACKED_C_H_INCLUDED_*/
