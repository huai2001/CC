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
#ifndef _C_CC_WIDGETS_IPLOCATOR_H_INCLUDED_
#define _C_CC_WIDGETS_IPLOCATOR_H_INCLUDED_

#include "dylib.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _cc_ip_locator _cc_ip_locator_t;

struct _cc_ip_locator {
    FILE* fp;
    uint32_t first_index;
    uint32_t last_index;
    uint32_t cur_start_ip;
    uint32_t cur_end_ip;
    uint32_t cur_end_ip_offset;
    uint32_t record_count;

    int32_t (*query)(_cc_ip_locator_t* f,
                     uint32_t ip,
                     byte_t* addr,
                     int32_t len);
    int32_t (*get_version)(_cc_ip_locator_t* f, byte_t* version, int32_t len);
    void (*quit)(_cc_ip_locator_t* f);
};

_CC_WIDGETS_API(bool_t) _cc_init_ip_locator(_cc_ip_locator_t*, const char_t*);
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_CRC_H_INCLUDED_*/
