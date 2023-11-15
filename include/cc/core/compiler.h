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

#ifndef _C_CC_CONFIG_COMPILER_H_INCLUDED_
#define _C_CC_CONFIG_COMPILER_H_INCLUDED_

#include "platform.h"

/* CPU Type Definition*/
#if defined(__x86_64) || defined(__x86_64__) || \
    defined(__amd64) || defined(__amd64__) || \
    defined(_M_AMD64) || defined(_M_X64)
    #define _CC_ARCH_X64__ 1
    #define _CC_ARCH_X86_64__ 1
    #define __CC_CPU64__ 1
#elif defined(i386) || defined(__i386) || defined(__i386__) || \
    defined(_X86_) || defined(__X86__) || defined(_M_IX86) || \
    defined(__I86__) || defined(__IA32__) || defined(__THW_INTEL)
    #define __CC_ARCH_X86__ 1
    #define __CC_ARCH_X86_32__ 1
    #define __CC_CPU32__ 1
#elif defined(__arm64) || defined(__arm64__) || \
    defined(__aarch64__) || defined(_M_ARM64)
    #define __CC_ARCH_ARM64__ 1
    #define __CC_CPU64__ 1
#elif defined(__arm) || defined(__arm__) || defined(_ARM_) || \
    defined(_ARM) || defined(_M_ARM) || defined(__TARGET_ARCH_ARM)
    #define __CC_ARCH_ARM32__ 1
    #define __CC_CPU32__ 1
#endif

#if !__CC_CPU64__ && __CC_CPU32__
    #if defined(_LP64) || defined(__LP64__) || defined(__64BIT__)
        #undef __CC_CPU32__
        #define __CC_CPU64__ 1
    #endif
#endif

#ifdef __cplusplus
    #if defined(__INTEL_COMPILER) && defined(_MSVC_LANG) && _MSVC_LANG < 201403L
        #if defined(__INTEL_CXX11_MODE__)
            #if defined(__cpp_aggregate_nsdmi)
                #define CXX_STD 201402L
            #else
                #define CXX_STD 201103L
            #endif
        #else
            #define CXX_STD 199711L
        #endif
    #elif defined(_MSC_VER) && defined(_MSVC_LANG)
        #define CXX_STD _MSVC_LANG
    #else
        #define CXX_STD __cplusplus
    #endif

    #if CXX_STD > 202002L
        #define _CC_STD_VERSION_ 23
    #elif CXX_STD > 201703L
        #define _CC_STD_VERSION_ 20
    #elif CXX_STD >= 201703L
        #define _CC_STD_VERSION_ 17
    #elif CXX_STD >= 201402L
        #define _CC_STD_VERSION_ 14
    #elif CXX_STD >= 201103L
        #define _CC_STD_VERSION_ 11
    #else
        #define _CC_STD_VERSION_ 98
    #endif

    #undef CXX_STD
/**
 * Some version of Standard C
 */
#elif !defined(__STDC__) && !defined(__clang__)
    #if defined(_MSC_VER) || defined(__ibmxl__) || defined(__IBMC__)
        # define _CC_STD_VERSION_ 89
    #else
        #define _CC_STD_VERSION_ 0
    #endif
#elif __STDC_VERSION__ > 201710L
    #define _CC_STD_VERSION_ 23
#elif __STDC_VERSION__ >= 201710L
    #define _CC_STD_VERSION_ 17
#elif __STDC_VERSION__ >= 201000L
    #define _CC_STD_VERSION_ 11
#elif __STDC_VERSION__ >= 199901L
    #define _CC_STD_VERSION_ 99
#else
    #define _CC_STD_VERSION_ 89
#endif

/** 
 * Define Unicode
 */
#if defined(UNICODE) || defined(_UNICODE)
#define _CC_UNICODE_ 1
#endif

/**
 * INTRIN_COMPILER_NAME.
 */
#define _CC_MACTOSTR(x) #x
#define _CC_MACROVALUESTR(x) _CC_MACTOSTR(x)

#ifdef __clang__
/**
 * Parameter '%0' not found in the function declaration
 */
#pragma clang diagnostic ignored "-Wdocumentation"
#endif


/* compiler builtin check (clang) */
#ifdef __has_builtin
    #define _cc_has_builtin(x) __has_builtin(x)
#else
    #define _cc_has_builtin(x) 0
#endif

/* compiler attribute check (gcc/clang) */
#ifdef __has_attribute
    #define _cc_has_attribute(x) __has_attribute(x)
#else
    #define _cc_has_attribute(x) 0
#endif

/* include check (gcc/clang) */
#ifdef __has_include
    #define _cc_has_include(x) __has_include(x)
#else
    #define _cc_has_include(x) 0
#endif

/* likely unlikely */
#if _cc_has_builtin(__builtin_expect) || __GNUC__ >= 4
    #define _cc_likely(expr) __builtin_expect(!!(expr), 1)
    #define _cc_unlikely(expr) __builtin_expect(!!(expr), 0)
#else
    #define _cc_likely(expr) (expr)
    #define _cc_unlikely(expr) (expr)
#endif

/**
 * Define compiler
 */
#if defined(__GNUC__)
    /* GNU C++ */
    #define _CC_GCC_ __GNUC__
    /* Macro for defining function name, file name, and line number */
    #define _CC_FUNC_ __func__
    #define _CC_FILE_ __FILE__
    #define _CC_LINE_ __LINE__

    #define _CC_INLINE_ __inline__

    #define _CC_COMPILER_VERSION_ __GNUC__

    #define _cc_align(x) __attribute__((aligned(x)))

    #if __GNUC__ < 4
        /* Added support for GCC-EMX <v4.x */
        /* this is needed for XFree86/OS2 developement */
        #ifdef __clang__
            #define _CC_FORCE_INLINE_ __attribute__((always_inline)) static _CC_INLINE_
        #endif

        #define _CC_API_EXPORT_ __declspec(export)
    #else
        #ifndef __CC_ANDROID__
            #define _CC_FORCE_INLINE_ __attribute__((always_inline)) static _CC_INLINE_
        #else
            #define _CC_FORCE_INLINE_ static _CC_INLINE_
        #endif

        #define _CC_API_EXPORT_ __attribute__ ((visibility ("default")))
    #endif

    #define _CC_API_IMPORT_

    /* ConceptGCC compiler:
    //   http://www.generic-programming.org/software/ConceptGCC/
    */
    #ifdef __GXX_CONCEPTS__
        #define _CC_COMPILER_NAME_ "ConceptGCC"
    #elif defined(__CYGWIN__)
        #define _CC_COMPILER_NAME_ "GCC(Cygmin)"
    #elif defined(__MINGW32__)
        #define _CC_COMPILER_NAME_ "GCC(MinGW)"
    #elif defined(__clang__)
        #define _CC_COMPILER_NAME_ "apple clang"
    #else
        #define _CC_COMPILER_NAME_ "GCC"
    #endif/* #  if defined(__CYGWIN__) */
    
#elif defined(_MSC_VER)
    /* Microsoft Visual C++
    //
    // Must remain the last #elif since some other vendors (Metrowerks, for example)
    // also #define _MSC_VER
    // Compiler version defines: VC6.0 : 1200, VC7.0 : 1300, VC7.1 : 1310, VC8.0 : 1400
    */
    #define _CC_MSVC_ _MSC_VER
    #define _CC_COMPILER_NAME_ "Microsoft VC++"
    
    /* Macro for defining function name, file name, and line number */
    #if _MSC_VER >= 1300
        #define _CC_FUNC_ __FUNCDNAME__
    #else
        #define _CC_FUNC_ __FUNCTION__
    #endif
    
    #define _CC_FILE_ __FILE__
    #define _CC_LINE_ __LINE__

    #define _CC_INLINE_ __inline
    #define _CC_FORCE_INLINE_ static __forceinline

    #define _cc_align(x) __declspec(align(x))

    #ifndef _CRT_SECURE_NO_DEPRECATE
        #pragma warning(disable: 4996) /*_CRT_SECURE_NO_WARNINGS*/
    #endif

    #pragma warning(disable: 4099)
    #pragma warning(disable: 4819)
 
    #define _CC_COMPILER_VERSION_ _MSC_VER
    /*
     * (_MSC_VER >= 1920) && (_MSC_VER < 2000) // Visual Studio 2019, MSVC++ 16.0
     * (_MSC_VER >= 1910) && (_MSC_VER < 1920) // Visual Studio 2017, MSVC++ 15.0
     * (_MSC_VER >= 1900) && (_MSC_VER < 1910) // Visual Studio 2015, MSVC++ 14.0
     * (_MSC_VER >= 1800) && (_MSC_VER < 1900) // Visual Studio 2013, MSVC++ 12.0
     * (_MSC_VER >= 1700) && (_MSC_VER < 1800) // Visual Studio 2012, MSVC++ 11.0
     * (_MSC_VER >= 1600) && (_MSC_VER < 1700) // Visual Studio 2010, MSVC++ 10.0
     * (_MSC_VER >= 1500) && (_MSC_VER < 1600) // Visual Studio 2008, MSVC++ 9.0
     * (_MSC_VER >= 1400) && (_MSC_VER < 1500) // Visual Studio 2005, MSVC++ 8.0
     * (_MSC_VER >= 1300) && (_MSC_VER < 1400) // Visual Studio 2003, MSVC++ 7.1
     * (_MSC_VER >= 1200) && (_MSC_VER < 1300) // Visual Studio 2002, MSVC++ 7.0
     * MSVC++ 5.0  _MSC_VER < 1200
     */
    #if _MSC_VER >= 1500
        #ifndef _CRT_SECURE_NO_WARNINGS
            #define _CRT_SECURE_NO_WARNINGS
        #endif
    #endif
    
    #if _MSC_VER >= 1900
        #ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
            #define _WINSOCK_DEPRECATED_NO_WARNINGS
        #endif
    #endif

    #define _CC_API_EXPORT_ __declspec(dllexport)
    #define _CC_API_IMPORT_ __declspec(dllimport)

    /* close windows.h min/max */
    #define NOMINMAX

#elif defined(__INTEL_COMPILER) || defined(__ICC)
//Intel
    #define _CC_COMPILER_NAME_ "intel c/c++"
#else
    #error unknown compiler
#endif

/* end Define compiler */


#if defined(_CC_EXPORT_SHARED_LIBRARY_)
    #define _CC_API_PUBLIC(t) _CC_API_EXPORT_ t 
#elif defined(_CC_IMPORT_SHARED_LIBRARY_)
    #define _CC_API_PUBLIC(t) _CC_API_IMPORT_ t
#else
    #define _CC_API_PUBLIC(t) t
#endif

#ifdef __CC_WINDOWS__
    #define _CC_API_PRIVATE(t) static t
#else
    #define _CC_API_PRIVATE(t) _CC_FORCE_INLINE_ t
#endif

#endif  /* _C_CC_CONFIG_COMPILER_H_INCLUDED_*/
