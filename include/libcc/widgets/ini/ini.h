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
#ifndef _C_CC_INI_H_INCLUDED_
#define _C_CC_INI_H_INCLUDED_

#include "../dylib.h"

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
    size_t length;
    tchar_t *name;

    union {
        bool_t uni_boolean;
        float64_t uni_float;
        int64_t uni_int;
        tchar_t *uni_string;
        _cc_rbtree_t uni_section;
    } element;
    _cc_rbtree_iterator_t lnk;
} _cc_ini_t;


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_INI_H_INCLUDED_*/
