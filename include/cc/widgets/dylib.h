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
#ifndef _C_CC_WIDGETS_DYLIB_H_INCLUDED_
#define _C_CC_WIDGETS_DYLIB_H_INCLUDED_

#include "../types.h"
#include "../logger.h"
#include "../list.h"
#include "../string.h"
#include "../event/event.h"
#include "../url.h"
#include "../rbtree.h"
#include "../buf.h"
#include "../math.h"
#include "../thread.h"

#if defined(_CC_WIDGETS_EXPORT_SHARED_LIBRARY_)
    #define _CC_WIDGETS_API(t) _CC_API_EXPORT_ t
#else
    #define _CC_WIDGETS_API(t) _CC_API_IMPORT_ t
#endif

#endif /*_C_CC_WIDGETS_DYLIB_H_INCLUDED_*/
