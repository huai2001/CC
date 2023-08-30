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
#ifndef _C_CC_WINDOWS_FLOCK_H_INCLUDED_
#define _C_CC_WINDOWS_FLOCK_H_INCLUDED_

#include "../windows.h"

#define LOCK_SH 1
#define LOCK_EX 2
#define LOCK_NB 4
#define LOCK_UN 8

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Set the file lock
 *
 * @param 1 file handle
 * @param 2 lock status:(LOCK_SH=1, LOCK_EX=2, LOCK_NB=4, LOCK_UN=8)
 *
 * @return true if successful or false on error.
 */
_CC_API(bool_t) flock(int, int32_t);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _C_CC_WINDOWS_FLOCK_H_INCLUDED_ */
