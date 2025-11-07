
#ifndef _C_CC_CONFIG_COMPILER_H_INCLUDED_
#define _C_CC_CONFIG_COMPILER_H_INCLUDED_

#include "os.h"

/* CPU Type Definition*/
#if defined(__x86_64) || defined(__x86_64__) || \
    defined(__amd64) || defined(__amd64__) || \
    defined(_M_AMD64) || defined(_M_X64)
    #define __CC_ARCH_X64__ 1
    #define __CC_ARCH_X86_64__ 1
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
    #if defined(_LP64) || defined(__LP64__) || defined(__64BIT__) || defined(_WIN64)
        #undef __CC_CPU32__
        #define __CC_CPU64__ 1
    #endif
#endif

#ifdef __cplusplus
    #if defined(__INTEL_COMPILER) && defined(_MSVC_LANG) && _MSVC_LANG < 201403L
        #if defined(__INTEL_CXX11_MODE__)
            #if defined(__cpp_aggregate_nsdmi)
                #define __CPP_VERSION__ 201402L
            #else
                #define __CPP_VERSION__ 201103L
            #endif
        #else
            #define __CPP_VERSION__ 199711L
        #endif
    #elif defined(__CC_MSVC__) && defined(_MSVC_LANG)
        #define __CPP_VERSION__ _MSVC_LANG
    #else
        #define __CPP_VERSION__ __cplusplus
    #endif

    #if __CPP_VERSION__ > 202002L
        #define __CC_STDC_VERSION__ 2023
    #elif __CPP_VERSION__ > 201703L
        #define __CC_STDC_VERSION__ 2017
    #elif __CPP_VERSION__ >= 201703L
        #define __CC_STDC_VERSION__ 2017
    #elif __CPP_VERSION__ >= 201402L
        #define __CC_STDC_VERSION__ 2014
    #elif __CPP_VERSION__ >= 201103L
        #define __CC_STDC_VERSION__ 2011
    #else
        #define __CC_STDC_VERSION__ 98
    #endif

    #undef __CPP_VERSION__
/**
 * Some version of Standard C
 */
#elif !defined(__STDC__) && !defined(__clang__)
    #if defined(__CC_MSVC__) || defined(__ibmxl__) || defined(__IBMC__)
        # define __CC_STDC_VERSION__ 89
    #else
        #define __CC_STDC_VERSION__ 0
    #endif
#elif __STDC_VERSION__ > 201710L
    #define __CC_STDC_VERSION__ 2023
#elif __STDC_VERSION__ >= 201710L
    #define __CC_STDC_VERSION__ 2017
#elif __STDC_VERSION__ >= 201000L
    #define __CC_STDC_VERSION__ 2011
#elif __STDC_VERSION__ >= 199901L
    #define __CC_STDC_VERSION__ 98
#else
    #define __CC_STDC_VERSION__ 89
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

#if defined(__GNUC__) || defined(__clang__)
  #define _cc_builtin_constant(x) __builtin_constant_p(x)
#else
  #define _cc_builtin_constant(x) (0)
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

#ifndef _cc_alignas
    #if defined(__cplusplus) && (__cplusplus >= 201103L)
        // Use C++11 standard alignas keyword if available
        #define _cc_alignas(n) alignas(n)
    #elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
        // Use C11 standard _Alignas keyword if available
        #define _cc_alignas(n) _Alignas(n)
    #elif defined(__GNUC__) || defined(__clang__)
        // Fallback to GCC/Clang attribute for alignment
        #define _cc_alignas(n) __attribute__((aligned(n)))
    #elif defined(_MSC_VER)
        // Fallback to MSVC declspec for alignment
        #define _cc_alignas(n) __declspec(align(n))
    #else
        // Error if no alignment mechanism is available
        #define _cc_alignas(n)
    #endif
#endif /* _cc_alignas not defined */

/**
 * Define compiler
 */
#ifdef __GNUC__
    /* GNU C++ */
    #define __CC_GNUC__ __GNUC__
    /* Macro for defining function name, file name, and line number */
    #define _CC_FUNC_ __func__
    #define _CC_FILE_ __FILE__
    #define _CC_LINE_ __LINE__

    #define _CC_INLINE_ __inline__

    #define _CC_COMPILER_VERSION_ __GNUC__

    #ifndef _cc_sync_synchronize
        #define _cc_sync_synchronize() __sync_synchronize()
    #endif
    
    /*A macro to demand a function be inlined.*/
    #ifndef __CC_ANDROID__
        #define _CC_FORCE_INLINE_ __attribute__((always_inline)) static inline
    #else
        #define _CC_FORCE_INLINE_ static inline
    #endif
    
    #ifndef _GNU_SOURCE
        #define _GNU_SOURCE
    #endif

    #if defined(__CC_MACOSX__) && !defined(_DARWIN_C_SOURCE)
        #define _DARWIN_C_SOURCE
    #endif

    #ifndef _POSIX_C_SOURCE
        #define _POSIX_C_SOURCE 200112L
    #endif

    /* Some compilers use a special export keyword */
    #ifndef __CC_WINDOWS__
        #define _CC_API_EXPORT_ __attribute__((visibility("default")))
        #define _CC_API_IMPORT_
        /**/
        #if __GNUC__ < 4
            #undef _CC_API_EXPORT_
            #define _CC_API_EXPORT_ __declspec(export)
        #endif
    #else
        #define _CC_API_EXPORT_ __declspec(dllexport)
        #define _CC_API_IMPORT_ __declspec(dllimport)
    #endif

    /* ConceptGCC compiler:
    //   http://www.generic-programming.org/software/ConceptGCC/
    */
    #ifdef __GXX_CONCEPTS__
        #define _CC_COMPILER_NAME_ "ConceptGCC"
    #elif defined(__CYGWIN__)
        #define _CC_COMPILER_NAME_ "GCC(Cygmin)"
    #elif defined(__CC_MINGW__)
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
    #define __CC_MSVC__ _MSC_VER
    #define _CC_COMPILER_NAME_ "Microsoft VC++"
    
    /* Macro for defining function name, file name, and line number */
    #if __CC_MSVC__ >= 1300
        #define _CC_FUNC_ __FUNCDNAME__
    #else
        #define _CC_FUNC_ __FUNCTION__
    #endif
    
    #define _CC_FILE_ __FILE__
    #define _CC_LINE_ __LINE__

    /*A macro to demand a function be inlined.*/
    #define _CC_INLINE_ __inline
    #define _CC_FORCE_INLINE_ static __forceinline
    
    #ifndef _cc_sync_synchronize
        #define _cc_sync_synchronize() _MemoryBarrier()
    #endif
    
    #ifndef _CRT_SECURE_NO_DEPRECATE
        #pragma warning(disable: 4996) /*_CRT_SECURE_NO_WARNINGS*/
    #endif

    #pragma warning(disable: 4099)
    #pragma warning(disable: 4819)
 
    #define _CC_COMPILER_VERSION_ __CC_MSVC__
    /*
     * (__CC_MSVC__ >= 1920) && (__CC_MSVC__ < 2000) // Visual Studio 2019, MSVC++ 16.0
     * (__CC_MSVC__ >= 1910) && (__CC_MSVC__ < 1920) // Visual Studio 2017, MSVC++ 15.0
     * (__CC_MSVC__ >= 1900) && (__CC_MSVC__ < 1910) // Visual Studio 2015, MSVC++ 14.0
     * (__CC_MSVC__ >= 1800) && (__CC_MSVC__ < 1900) // Visual Studio 2013, MSVC++ 12.0
     * (__CC_MSVC__ >= 1700) && (__CC_MSVC__ < 1800) // Visual Studio 2012, MSVC++ 11.0
     * (__CC_MSVC__ >= 1600) && (__CC_MSVC__ < 1700) // Visual Studio 2010, MSVC++ 10.0
     * (__CC_MSVC__ >= 1500) && (__CC_MSVC__ < 1600) // Visual Studio 2008, MSVC++ 9.0
     * (__CC_MSVC__ >= 1400) && (__CC_MSVC__ < 1500) // Visual Studio 2005, MSVC++ 8.0
     * (__CC_MSVC__ >= 1300) && (__CC_MSVC__ < 1400) // Visual Studio 2003, MSVC++ 7.1
     * (__CC_MSVC__ >= 1200) && (__CC_MSVC__ < 1300) // Visual Studio 2002, MSVC++ 7.0
     * MSVC++ 5.0  __CC_MSVC__ < 1200
     */
    #if __CC_MSVC__ >= 1500
        #ifndef _CRT_SECURE_NO_WARNINGS
            #define _CRT_SECURE_NO_WARNINGS
        #endif
    #endif
    
    #if __CC_MSVC__ >= 1900
        #ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
            #define _WINSOCK_DEPRECATED_NO_WARNINGS
        #endif
    #endif

    /* Some compilers use a special export keyword */
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

/* By default libcc uses the C calling convention */
#ifndef __CC_CALL__
    #if defined(__CC_WINDOWS__) && !defined(__GNUC__)
        #define __CC_CALL__ __cdecl
    #else
        #define __CC_CALL__
    #endif
#endif /* __CC_CALL__ */

#if defined(_CC_API_USE_DYNAMIC_)
    #define _CC_API_PUBLIC(t) _CC_API_EXPORT_ t 
#elif defined(_CC_API_IMPORT_DYNAMIC_)
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
