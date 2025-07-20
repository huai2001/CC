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
#ifndef _C_CC_VERSION__H_INCLUDED_
#define _C_CC_VERSION__H_INCLUDED_

#include "core/compiler.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** @name Version Number
 *  Printable format: "%d.%d.%d", MAJOR, MINOR, MICRO
*/
#define _CC_MAJOR_VERSION_    3
#define _CC_MINOR_VERSION_    0
#define _CC_MICRO_VERSION_    0

#define _CC_VERSION_ "3.0.0"

/**
 * This macro turns the version numbers into a numeric value.
 *
 * (1,2,3) becomes 1002003.
 *
 * \param major the major version number.
 * \param minor the minorversion number.
 * \param patch the patch version number.
 *
 */
#define _CC_VERSIONNUM(major, minor, patch) \
    ((major) * 1000000 + (minor) * 1000 + (patch))

_CC_FORCE_INLINE_ int _cc_get_version(void) {
    return _CC_VERSIONNUM(_CC_MAJOR_VERSION_, _CC_MINOR_VERSION_, _CC_MICRO_VERSION_);
}
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_VERSION__H_INCLUDED_*/
