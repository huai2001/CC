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
#ifndef _C_CC_TYPES_H_INCLUDED_
#define _C_CC_TYPES_H_INCLUDED_

#define HAVE_ASSERT_H

#include <stddef.h>
#include <ctype.h>
#include <wctype.h>
#include <limits.h>
#include "tchar.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Apparently this is needed by several Windows compilers */
#if !defined(__MACH__)
    /* Define NULL pointer value */
    #ifndef NULL
        #ifdef __cplusplus
            #define NULL 0
        #else
            #define NULL ((void *)0)
        #endif
    #endif
#endif /* NULL */

/**
 *  name Basic data types
 */

/*@{*/
typedef char char_t;
typedef unsigned char uchar_t;
typedef uchar_t byte_t;

#ifdef __CC_WINDOWS__
#if _CC_MSVC_ >= 1700
    #include <stdint.h>
#elif _CC_MSVC_ < 1300
    typedef signed char int8_t;
    typedef signed short int16_t;
    typedef signed int int32_t;
    typedef signed __int64 int64_t;
    typedef unsigned char uint8_t;
    typedef unsigned short uint16_t;
    typedef unsigned int uint32_t;
    typedef unsigned __int64 uint64_t;
#else
    typedef signed __int8 int8_t;
    typedef signed __int16 int16_t;
    typedef signed __int32 int32_t;
    typedef signed __int64 int64_t;
    typedef unsigned __int8 uint8_t;
    typedef unsigned __int16 uint16_t;
    typedef unsigned __int32 uint32_t;
    typedef unsigned __int64 uint64_t;
#endif
#elif (defined(__sun) || defined(__sun__)) && defined(__SunOS_5_9)
    #include <sys/inttypes.h>
#else
    #include <stdint.h>
#endif

/*
int8_t   - [-128 : 127]
int16_t  - [-32768 : 32767]
int32_t  - [-2147483648 : 2147483647]
int64_t  - [-9223372036854775808 : 9223372036854775807]

uint8_t  - [0 : 255]
uint16_t - [0 : 65535]
uint32_t - [0 : 4294967295]
uint64_t - [0 : 18446744073709551615]
*/

#ifdef __cplusplus
    typedef bool bool_t;
#elif defined(__bool_true_false_are_defined)
    typedef _Bool bool_t;
#else
    #undef false
    #undef true
    typedef enum {
        false = 0,
        true = 1
    } bool_t;
#endif

/**/
#ifndef float32_t
    typedef float float32_t;
#endif

/**/
#ifndef float64_t
    typedef double float64_t;
#endif

/**/
#ifndef pvoid_t
    typedef void *pvoid_t;
#endif

#ifndef nullptr
    #define nullptr NULL
#endif
    
#ifndef intptr_t
    /**/
    #ifndef _W64
        #define _W64
    #endif

    #ifdef __CC_CPU64__
        typedef int64_t intptr_t;
        typedef uint64_t uintptr_t;
    #elif defined(__CC_CPU32__)
        typedef _W64 int32_t intptr_t;
        typedef _W64 uint32_t uintptr_t;
    #else
        typedef _W64 int32_t intptr_t;
        typedef _W64 uint32_t uintptr_t;
    #endif
#endif

/**/
#ifndef _CC_UNICODE_
    typedef char_t tchar_t;
#else
    typedef wchar_t tchar_t;
#endif

/**/
typedef uint16_t us2_t;
typedef uint32_t us4_t;

#define _CC_RC_ERROR_    0
#define _CC_RC_OK_       1
#define _CC_YES_         _CC_RC_OK_
#define _CC_NO_          _CC_RC_ERROR_

/*@}*//*Basic data types*/

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}    /* ... extern "C" */
#endif

#endif /* _C_CC_TYPES_H_INCLUDED_ */
