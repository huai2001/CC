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

#ifndef _C_CC_CPU_INFO_H_INCLUDED_
#define _C_CC_CPU_INFO_H_INCLUDED_

#include "../types.h"

/* Need to do this here because intrin.h has C++ code in it */
/* Visual Studio 2005 has a bug where intrin.h conflicts with winnt.h */
#if defined(_CC_MSVC_) && (_CC_MSVC_ >= 1500) && (defined(_M_IX86) || defined(_M_X64))
    #include <intrin.h>

    #ifndef __CC_CPU64__
        #define __MMX__
        #define __3dNOW__
    #endif

    #define __SSE__
    #define __SSE2__

#elif defined(__MINGW64_VERSION_MAJOR)
    #include <intrin.h>
#else
    #ifdef __ALTIVEC__
        #if HAVE_ALTIVEC_H && !defined(__APPLE_ALTIVEC__)
            #include <altivec.h>
            #undef pixel
        #endif
    #endif

    #ifdef __MMX__
        #include <mmintrin.h>
    #endif

    #ifdef __3dNOW__
        #include <mm3dnow.h>
    #endif

    #ifdef __SSE__
        #include <xmmintrin.h>
    #endif

    #ifdef __SSE2__
        #include <emmintrin.h>
    #endif
#endif

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**/
extern int _cc_cpu_number_processors;

/**
 *  @brief This function returns the number of CPU cores available.
 */
_CC_API_PUBLIC(int) _cc_cpu_count(void);

_CC_API_PUBLIC(const tchar_t*) _cc_cpu_sn(void);


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _C_CC_CPU_INFO_H_INCLUDED_ */

