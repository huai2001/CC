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
#ifndef _C_CC_WIDGETS_MEMORY_H_INCLUDED_
#define _C_CC_WIDGETS_MEMORY_H_INCLUDED_

#include "dylib.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif
   
/**/
_CC_WIDGETS_API(pvoid_t) _cc_mem_calloc(size_t, size_t,const tchar_t*, const int, const tchar_t*);
/**/
_CC_WIDGETS_API(pvoid_t) _cc_mem_malloc(size_t,const tchar_t*, const int, const tchar_t*);
/**/
_CC_WIDGETS_API(pvoid_t) _cc_mem_realloc(pvoid_t,size_t,const tchar_t*, const int, const tchar_t*);
/**/
_CC_WIDGETS_API(void) _cc_mem_free(pvoid_t,const tchar_t*, const int, const tchar_t*);
/**/
_CC_WIDGETS_API(char_t*) _cc_mem_strdupA(const char_t*,const tchar_t*, const int, const tchar_t*);
/**/
_CC_WIDGETS_API(wchar_t*) _cc_mem_strdupW(const wchar_t*,const tchar_t*, const int, const tchar_t*);
/**/
_CC_WIDGETS_API(char_t*) _cc_mem_strndupA(const char_t*,size_t,const tchar_t*, const int, const tchar_t*);
/**/
_CC_WIDGETS_API(wchar_t*) _cc_mem_strndupW(const wchar_t*,size_t,const tchar_t*, const int, const tchar_t*);


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif 

#endif/*_C_CC_MEMORY_H_INCLUDED_*/
