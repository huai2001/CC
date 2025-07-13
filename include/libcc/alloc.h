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
#ifndef _C_CC_MEMORY_H_INCLUDED_
#define _C_CC_MEMORY_H_INCLUDED_

#include "logger.h"
#include "list.h"
#include "string.h"

#if defined(__CC_USE_TCMALLOC__)
    #include <google/tcmalloc.h>
    #if (TC_VERSION_MAJOR == 1 && TC_VERSION_MINOR >= 6) || (TC_VERSION_MAJOR > 1)
        #define _cc_malloc_size(p) tc_malloc_size(p)
    #else
        #error "Newer version of tcmalloc required"
    #endif
#elif defined(__CC_USE_JEMALLOC__)
    #include <jemalloc/jemalloc.h>
    #if (JEMALLOC_VERSION_MAJOR == 2 && JEMALLOC_VERSION_MINOR >= 1) || (JEMALLOC_VERSION_MAJOR > 2)
        #define _cc_malloc_size(p) je_malloc_usable_size(p)
    #else
        #error "Newer version of jemalloc required"
    #endif
#elif defined(__CC_APPLE__)
    #include <malloc/malloc.h>
    #define _cc_malloc_size(p) malloc_size(p)
#endif
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#ifdef _CC_DEBUG_
#define _CC_ENABLE_MEMORY_TRACKED_ 1
#endif

/**/
_CC_API_PUBLIC(pvoid_t) _cc_malloc(size_t);
/**/
_CC_API_PUBLIC(pvoid_t) _cc_calloc(size_t, size_t);
/**/
_CC_API_PUBLIC(pvoid_t) _cc_realloc(pvoid_t, size_t);
/**/
_CC_API_PUBLIC(void) _cc_free(pvoid_t);
/**/
_CC_API_PUBLIC(char_t*) _cc_strdupA(const char_t*);
/**/
_CC_API_PUBLIC(wchar_t*) _cc_strdupW(const wchar_t*);

/**/
_CC_API_PUBLIC(char_t*) _cc_strndupA(const char_t*,size_t);
/**/
_CC_API_PUBLIC(wchar_t*) _cc_strndupW(const wchar_t*,size_t);

#ifdef _CC_ENABLE_MEMORY_TRACKED_
/**/
_CC_API_PUBLIC(void) _cc_enable_tracked_memory(void);
#else
#define _cc_enable_tracked_memory()
#endif

/**/
#define _CC_MALLOCX(T,C) ((T*)_cc_malloc(sizeof(T) * (C)))
#define _CC_MALLOC(T) ((T*)_cc_malloc(sizeof(T)))
#define _CC_CALLOC(T,C) ((T*)_cc_calloc(C,sizeof(T)))

/**/
#ifdef _CC_UNICODE_
    #define _cc_tcsdup(d)       _cc_strdupW(d)
    #define _cc_tcsndup(d,n)    _cc_strndupW(d,n)
#else
    #define _cc_tcsdup(d)       _cc_strdupA(d)
    #define _cc_tcsndup(d,n)     _cc_strndupA(d,n)
#endif
    
/**/
#define _cc_safe_free(d) do {\
    if ((d)) {\
        _cc_free((d));\
        (d)=nullptr;\
    }\
} while (0)

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif 

#endif/*_C_CC_MEMORY_H_INCLUDED_*/
