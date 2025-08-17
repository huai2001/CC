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
#ifndef _C_CC_ENDIAN_H_INCLUDED_
#define _C_CC_ENDIAN_H_INCLUDED_

#include "generic.h"

#ifdef __CC_LINUX__
    #include <endian.h>
#elif defined(__CC_MACOSX__)
    #include <libkern/OSByteOrder.h>
#elif defined(__CC_FREEBSD__) || defined(__CC_OPENBSD__) || defined(__CC_NETBSD__)
    #include <sys/endian.h>
#elif defined(__CC_SOLARIS__) && !defined(__CC_WINDOWS__) && !defined(__PPC__)
    #include <byteswap.h>
#endif

/* Byte ordering detection */
/* This will likely define BYTE_ORDER */
#include <sys/types.h> 

#ifndef BYTE_ORDER
    #if (BSD >= 199103)
        #include <machine/endian.h>
    #else
        #if defined(linux) || defined(__linux__)
            # include <endian.h>
        #else
            /* least-significant byte first (vax, pc) */
            #define LITTLE_ENDIAN   1234
            /* most-significant byte first (IBM, net) */
            #define BIG_ENDIAN  4321
            /* LSB first in word, MSW first in long (pdp)*/
            #define PDP_ENDIAN  3412

            #if defined(__i386__) || defined(__x86_64__) || defined(__amd64__) || \
                defined(vax) || defined(ns32000) || defined(sun386) || \
                defined(MIPSEL) || defined(_MIPSEL) || defined(BIT_ZERO_ON_RIGHT) || \
                defined(__alpha__) || defined(__alpha)
                #define BYTE_ORDER    LITTLE_ENDIAN
            #endif

            #if defined(sel) || defined(pyr) || defined(mc68000) || defined(sparc) || \
                defined(is68k) || defined(tahoe) || defined(ibm032) || defined(ibm370) || \
                defined(MIPSEB) || defined(_MIPSEB) || defined(_IBMR2) || defined(DGUX) ||\
                defined(apollo) || defined(__convex__) || defined(_CRAY) || \
                defined(__hppa) || defined(__hp9000) || \
                defined(__hp9000s300) || defined(__hp9000s700) || \
                defined (BIT_ZERO_ON_LEFT) || defined(m68k) || defined(__sparc)
                #define BYTE_ORDER  BIG_ENDIAN
            #endif
        #endif /* linux */
    #endif /* BSD */
#endif /* BYTE_ORDER */

/* Sometimes after including an OS-specific header that defines the
 * endianness we end with __BYTE_ORDER but not with BYTE_ORDER that is what
 * the Redis code uses. In this case let's define everything without the
 * underscores. */
#ifndef BYTE_ORDER
    #ifdef __BYTE_ORDER
        #if defined(__LITTLE_ENDIAN) && defined(__BIG_ENDIAN)
            #ifndef LITTLE_ENDIAN
                #define LITTLE_ENDIAN __LITTLE_ENDIAN
            #endif

            #ifndef BIG_ENDIAN
                #define BIG_ENDIAN __BIG_ENDIAN
            #endif

            #if (__BYTE_ORDER == __LITTLE_ENDIAN)
                #define BYTE_ORDER LITTLE_ENDIAN
            #else
                #define BYTE_ORDER BIG_ENDIAN
            #endif
        #endif
    #endif
#endif /* BYTE_ORDER */

#if !defined(BYTE_ORDER) || (BYTE_ORDER != BIG_ENDIAN && BYTE_ORDER != LITTLE_ENDIAN)
    #define BYTE_ORDER LITTLE_ENDIAN
#endif

#define _CC_BYTEORDER_ BYTE_ORDER
#define _CC_LIL_ENDIAN_ LITTLE_ENDIAN
#define _CC_BIG_ENDIAN_ BIG_ENDIAN
#define _CC_PDP_ENDIAN_ PDP_ENDIAN

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* various modern compilers may have builtin swap */
#if defined(__GNUC__) || defined(__clang__)
    #define HAS_BUILTIN_BSWAP16 (_cc_has_builtin(__builtin_bswap16)) || (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8))
    #define HAS_BUILTIN_BSWAP32 (_cc_has_builtin(__builtin_bswap32)) || (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3))
    #define HAS_BUILTIN_BSWAP64 (_cc_has_builtin(__builtin_bswap64)) || (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3))
    /* this one is broken */
    #define HAS_BROKEN_BSWAP (__GNUC__ == 2 && __GNUC_MINOR__ <= 95)
#else
    #define HAS_BUILTIN_BSWAP16      0
    #define HAS_BUILTIN_BSWAP32      0
    #define HAS_BUILTIN_BSWAP64      0
    #define HAS_BROKEN_BSWAP         0
#endif
/* Byte swap 16-bit integer. */
#if HAS_BUILTIN_BSWAP16
#define _cc_swap16(x) __builtin_bswap16(x)
#elif (defined(_MSC_VER) && (_MSC_VER >= 1400)) && !defined(__ICL)
#pragma intrinsic(_byteswap_ushort)
#define _cc_swap16(x) _byteswap_ushort(x)
#elif defined(__i386__) && !HAS_BROKEN_BSWAP
_CC_FORCE_INLINE_ uint16_t _cc_swap16(uint16_t x) {
  __asm__("xchgb %b0,%h0": "=q"(x):"0"(x));
    return x;
}
#elif defined(__x86_64__)
_CC_FORCE_INLINE_ uint16_t _cc_swap16(uint16_t x) {
  __asm__("xchgb %b0,%h0": "=Q"(x):"0"(x));
    return x;
}
#elif (defined(__powerpc__) || defined(__ppc__))
_CC_FORCE_INLINE_ uint16_t _cc_swap16(uint16_t x) {
    int result;

  __asm__("rlwimi %0,%2,8,16,23": "=&r"(result):"0"(x >> 8), "r"(x));
    return (uint16_t)result;
}
#elif (defined(__m68k__) && !defined(__mcoldfire__))
_CC_FORCE_INLINE_ uint16_t _cc_swap16(uint16_t x) {
  __asm__("rorw #8,%0": "=d"(x): "0"(x):"cc");
    return x;
}
#elif defined(__WATCOMC__) && defined(__386__)
extern __inline uint16_t _cc_swap16(uint16_t);
#pragma aux _cc_swap16 = "xchg al, ah" parm[ax] modify[ax];
#else
_CC_FORCE_INLINE_ uint16_t _cc_swap16(uint16_t x) {
    return (uint16_t)(((x&0xFF) << 8) | ((x&0xFF00) >> 8));
}
#endif

/* Byte swap 32-bit integer. */
#if HAS_BUILTIN_BSWAP32
#define _cc_swap32(x) __builtin_bswap32(x)
#elif (defined(_MSC_VER) && (_MSC_VER >= 1400)) && !defined(__ICL)
#pragma intrinsic(_byteswap_ulong)
#define _cc_swap32(x) _byteswap_ulong(x)
#elif defined(__i386__) && !HAS_BROKEN_BSWAP
_CC_FORCE_INLINE_ uint32_t _cc_swap32(uint32_t x) {
  __asm__("bswap %0": "=r"(x):"0"(x));
    return x;
}
#elif defined(__x86_64__)
_CC_FORCE_INLINE_ uint32_t _cc_swap32(uint32_t x) {
  __asm__("bswapl %0": "=r"(x):"0"(x));
    return x;
}
#elif (defined(__powerpc__) || defined(__ppc__))
_CC_FORCE_INLINE_ uint32_t _cc_swap32(uint32_t x) {
    uint32_t result;
  __asm__("rlwimi %0,%2,24,16,23": "=&r"(result): "0" (x>>24),  "r"(x));
  __asm__("rlwimi %0,%2,8,8,15"  : "=&r"(result): "0" (result), "r"(x));
  __asm__("rlwimi %0,%2,24,0,7"  : "=&r"(result): "0" (result), "r"(x));
    return result;
}
#elif (defined(__m68k__) && !defined(__mcoldfire__))
_CC_FORCE_INLINE_ uint32_t _cc_swap32(uint32_t x) {
  __asm__("rorw #8,%0\n\tswap %0\n\trorw #8,%0": "=d"(x): "0"(x):"cc");
    return x;
}
#elif defined(__WATCOMC__) && defined(__386__)
extern _CC_INLINE_ uint32_t _cc_swap32(uint32_t);
#pragma aux _cc_swap32 = "bswap eax" parm[eax] modify[eax];
#else
_CC_FORCE_INLINE_ uint32_t _cc_swap32(uint32_t x) {
    return (uint32_t)((x << 24) | ((x << 8) & 0x00FF0000) | ((x >> 8) & 0x0000FF00) | (x >> 24));
}
#endif

/* Byte swap 64-bit integer. */
#if HAS_BUILTIN_BSWAP64
#define _cc_swap64(x) __builtin_bswap64(x)
#elif (defined(_MSC_VER) && (_MSC_VER >= 1400)) && !defined(__ICL)
#pragma intrinsic(_byteswap_uint64)
#define _cc_swap64(x) _byteswap_uint64(x)
#elif defined(__i386__) && !HAS_BROKEN_BSWAP
_CC_FORCE_INLINE_ uint64_t _cc_swap64(uint64_t x) {
    union {
        struct {
            uint32_t a, b;
        } s;
        uint64_t u;
    } v;
    v.u = x;
  __asm__("bswapl %0 ; bswapl %1 ; xchgl %0,%1"
          : "=r"(v.s.a), "=r"(v.s.b)
          : "0" (v.s.a),  "1"(v.s.b));
    return v.u;
}
#elif defined(__x86_64__)
_CC_FORCE_INLINE_ uint64_t _cc_swap64(uint64_t x) {
  __asm__("bswapq %0": "=r"(x):"0"(x));
    return x;
}
#elif defined(__WATCOMC__) && defined(__386__)
extern __inline uint64_t _cc_swap64(uint64_t);
#pragma aux _cc_swap64 = \
  "bswap eax"     \
  "bswap edx"     \
  "xchg eax,edx"  \
  parm [eax edx]  \
  modify [eax edx];
#else
_CC_FORCE_INLINE_ uint64_t _cc_swap64(uint64_t x) {
    uint32_t hi, lo;
    /* Separate into high and low 32-bit values and swap them */
    lo = (uint32_t)(x & 0xFFFFFFFF);
    x >>= 32;
    hi = (uint32_t)(x & 0xFFFFFFFF);
    x = _cc_swap32(lo);
    x <<= 32;
    x |= _cc_swap32(hi);
    return (x);
}
#endif

/* remove extra macros */
#undef HAS_BROKEN_BSWAP
#undef HAS_BUILTIN_BSWAP16
#undef HAS_BUILTIN_BSWAP32
#undef HAS_BUILTIN_BSWAP64

/**/
_CC_FORCE_INLINE_ float _cc_swap_float(float x) {
    union {
        float f;
        uint32_t ui32;
    } swapper;

    swapper.f = x;
    swapper.ui32 = _cc_swap32(swapper.ui32);
    return swapper.f;
}

/**/
_CC_FORCE_INLINE_ bool_t _cc_isbig(void) {
    static union {
        unsigned short s;
        unsigned char c;
    } u = { 1 };
    
    return u.c == 0;
}

/**
 *  \name Swap to native
 *  Byteswap item from the specified endianness to the native endianness.
 */
/* @{ */
#if _CC_BYTEORDER_ == _CC_LIL_ENDIAN_
    #define _CC_SWAPLE16(X) (X)
    #define _CC_SWAPLE32(X) (X)
    #define _CC_SWAPLE64(X) (X)
    #define _CC_SWAPFLOATLE(X) (X)
    #define _CC_SWAPBE16(X) _cc_swap16(X)
    #define _CC_SWAPBE32(X) _cc_swap32(X)
    #define _CC_SWAPBE64(X) _cc_swap64(X)
    #define _CC_SWAPFLOATBE(X) _cc_swap_float(X)
#else
    #define _CC_SWAPLE16(X) _cc_swap16(X)
    #define _CC_SWAPLE32(X) _cc_swap32(X)
    #define _CC_SWAPLE64(X) _cc_swap64(X)
    #define _CC_SWAPFLOATLE(X) _cc_swap_float(X)
    #define _CC_SWAPBE16(X) (X)
    #define _CC_SWAPBE32(X) (X)
    #define _CC_SWAPBE64(X) (X)
    #define _CC_SWAPFLOATBE(X) (X)
#endif
/* @} *//* Swap to native */

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _C_CC_ENDIAN_H_INCLUDED_ */
