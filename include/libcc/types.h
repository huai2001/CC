#ifndef _C_CC_TYPES_H_INCLUDED_
#define _C_CC_TYPES_H_INCLUDED_

#define HAVE_ASSERT_H

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>
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
#if __CC_MSVC__ >= 1700
    #include <stdint.h>
#elif __CC_MSVC__ < 1300
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

/**/
#ifndef _CC_UNICODE_
    typedef char_t      tchar_t;
#else
    typedef wchar_t     tchar_t;
#endif

/*@}*//*Basic data types*/

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}    /* ... extern "C" */
#endif

#endif /* _C_CC_TYPES_H_INCLUDED_ */
