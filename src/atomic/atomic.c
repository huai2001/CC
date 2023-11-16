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
#include <cc/atomic.h>
#if 0
_CC_API_PRIVATE(int32_t) _xxx_atomic32_set(_cc_atomic32_t* a, int32_t v) {
    int32_t value;
    do {
        value = *a;
    } while (!_cc_atomic32_cas(a, value, v));
    return value;
}

_CC_API_PRIVATE(int64_t) _xxx_atomic64_set(_cc_atomic64_t* a, int64_t v) {
    int64_t value;
    do {
        value = *a;
    } while (!_cc_atomic64_cas(a, value, v));
    return value;
}
#endif

_CC_API_PUBLIC(int32_t) _cc_atomic32_get(_cc_atomic32_t* a) {
    _cc_atomic32_t value;
    do {
        value = (int32_t)*a;
    } while (!_cc_atomic32_cas(a, value, value));

    return value;
}

_CC_API_PUBLIC(int64_t) _cc_atomic64_get(_cc_atomic64_t* a) {
    int64_t value;
    do {
        value = (int64_t)*a;
    } while (!_cc_atomic64_cas(a, value, value));

    return value;
}

_CC_API_PUBLIC(int32_t) _cc_atomic32_add(_cc_atomic32_t* a, int32_t v) {
#if defined(__CC_WINDOWS__)
#if (defined(_M_IA64) || defined(_M_AMD64))
    return (int32_t)InterlockedExchangeAdd(a, v);
#else
    return (int32_t)InterlockedExchangeAdd((long*)a, v);
#endif
#elif defined(__CC_MACOSX__) || defined(__CC_IPHONEOS__)
    // return OSAtomicAdd32Barrier(v, a);
    return atomic_fetch_add(a, v);
#elif defined(__CC_SOLARIS__)
    int32_t pv = (int32_t)*a;
    membar_consumer();
    atomic_add_32(a, v);
    return pv;
#else
    return __sync_fetch_and_add(a, v);
#endif
}

_CC_API_PUBLIC(int32_t) _cc_atomic32_sub(_cc_atomic32_t* a, int32_t v) {
#if defined(__CC_WINDOWS__)
#if (defined(_M_IA64) || defined(_M_AMD64))
    return (int32_t)InterlockedExchangeAdd(a, -v);
#else
    return (int32_t)InterlockedExchangeAdd((long*)a, -v);
#endif
#elif defined(__CC_MACOSX__) || defined(__CC_IPHONEOS__)
    // return OSAtomicAdd32Barrier(-v, a);
    return atomic_fetch_sub(a, v);
#elif defined(__CC_SOLARIS__)
    int32_t pv = (int32_t)*a;
    membar_consumer();
    atomic_add_32((volatile uint32_t*)a, -v);
    return pv;
#else
    return __sync_fetch_and_sub(a, v);
#endif
}

_CC_API_PUBLIC(int32_t) _cc_atomic32_set(_cc_atomic32_t* a, int32_t v) {
#if defined(__CC_WINDOWS__)
#if (defined(_M_IA64) || defined(_M_AMD64)) && !defined(RC_INVOKED)
    return (int32_t)InterlockedExchange(a, v);
#else
    return (int32_t)InterlockedExchange((long*)a, v);
#endif
#elif defined(__CC_MACOSX__) || defined(__CC_IPHONEOS__)
    // return _xxx_atomic32_set(a, v);
    return atomic_exchange(a, v);
#elif defined(__CC_SOLARIS__)
    return atomic_swap_32(a, v);
#else
    return __sync_lock_test_and_set(a, v);
#endif
}

_CC_API_PUBLIC(int32_t) _cc_atomic32_and(_cc_atomic32_t* a, int32_t v) {
#if defined(__CC_WINDOWS__)
    return (int32_t)_InterlockedAnd(a, v);
#elif defined(__CC_MACOSX__) || defined(__CC_IPHONEOS__)
    // return OSAtomicAnd32((uint32_t)v, (volatile uint32_t*)a);
    return atomic_fetch_and(a, v);
#elif defined(__CC_SOLARIS__)
#error _cc_atomic32_and Unsupported OS
    return 0;
#else
    return __sync_and_and_fetch(a, v);
#endif
}

_CC_API_PUBLIC(int32_t) _cc_atomic32_or(_cc_atomic32_t* a, int32_t v) {
#if defined(__CC_WINDOWS__)
    return (int32_t)_InterlockedOr(a, v);
#elif defined(__CC_MACOSX__) || defined(__CC_IPHONEOS__)
    // return OSAtomicOr32((uint32_t)v, (volatile uint32_t*)a);
    return atomic_fetch_or(a, v);
#elif defined(__CC_SOLARIS__)
#error _cc_atomic32_or Unsupported OS
    return 0;
#else
    return __sync_fetch_and_or(a, v);
#endif
}

_CC_API_PUBLIC(int32_t) _cc_atomic32_xor(_cc_atomic32_t* a, int32_t v) {
#if defined(__CC_WINDOWS__)
    return (int32_t)_InterlockedXor(a, v);
#elif defined(__CC_MACOSX__) || defined(__CC_IPHONEOS__)
    // return OSAtomicXor32((uint32_t)v, (volatile uint32_t*)a);
    return atomic_fetch_xor(a, v);
#elif defined(__CC_SOLARIS__)
#error _cc_atomic32_xor Unsupported OS
    return 0;
#else
    return __sync_fetch_and_xor(a, v);
#endif
}

_CC_API_PUBLIC(bool_t) _cc_atomic32_cas(_cc_atomic32_t* a, int32_t v1, int32_t v2) {
#if defined(__CC_WINDOWS__)
#if (defined(_M_IA64) || defined(_M_AMD64)) && !defined(RC_INVOKED)
    return (bool_t)(InterlockedCompareExchange(a, v2, v1) == v1);
#else
    return (bool_t)(InterlockedCompareExchange((long*)a, v2, v1) == v1);
#endif
#elif defined(__CC_MACOSX__) || defined(__CC_IPHONEOS__)
    // return (bool_t)OSAtomicCompareAndSwap32Barrier(v1, v2, a);
    return (bool_t)atomic_compare_exchange_strong(a, &v1, v2);
#elif defined(__CC_SOLARIS__)
    return (bool_t)(atomic_cas_ptr(a, v1, v2) == v1);
#else
    return (bool_t)__sync_bool_compare_and_swap(a, v1, v2);
#endif
}

_CC_API_PUBLIC(int64_t) _cc_atomic64_add(_cc_atomic64_t* a, int64_t v) {
#if defined(__CC_WINDOWS__)
    return (int64_t)InterlockedExchangeAdd64(a, v);
#elif defined(__CC_MACOSX__) || defined(__CC_IPHONEOS__)
    // return OSAtomicAdd64Barrier(v, a);
    return atomic_fetch_add(a, v);
#elif defined(__CC_SOLARIS__)
    int64_t pv = (int64_t)*a;
    membar_consumer();
    atomic_add_64(a, v);
    return pv;
#else
    return __sync_fetch_and_add(a, v);
#endif
}

_CC_API_PUBLIC(int64_t) _cc_atomic64_sub(_cc_atomic64_t* a, int64_t v) {
#if defined(__CC_WINDOWS__)
    return (int64_t)InterlockedExchangeAdd64(a, -v);
#elif defined(__CC_MACOSX__) || defined(__CC_IPHONEOS__)
    // return OSAtomicAdd64Barrier(-v, a);
    return atomic_fetch_sub(a, v);
#elif defined(__CC_SOLARIS__)
    int64_t pv = (int64_t)*a;
    membar_consumer();
    atomic_add_64((volatile uint64_t*)a, -v);
    return pv;
#else
    return __sync_fetch_and_sub(a, v);
#endif
}

_CC_API_PUBLIC(int64_t) _cc_atomic64_set(_cc_atomic64_t* a, int64_t v) {
#if defined(__CC_WINDOWS__)
    return (int64_t)InterlockedExchange64(a, v);
#elif defined(__CC_MACOSX__) || defined(__CC_IPHONEOS__)
    // return _xxx_atomic64_set(a, v);
    return atomic_exchange(a, v);
#elif defined(__CC_SOLARIS__)
    return atomic_swap_64(a, v);
#else
    return __sync_lock_test_and_set(a, v);
#endif
}

_CC_API_PUBLIC(int64_t) _cc_atomic64_and(_cc_atomic64_t* a, int64_t v) {
#if defined(__CC_WINDOWS__)
    return (int64_t)InterlockedAnd64(a, v);
#elif defined(__CC_MACOSX__) || defined(__CC_IPHONEOS__)
    // return OSAtomicAnd32((uint32_t)v, (volatile uint32_t*)a);
    return atomic_fetch_and(a, v);
#elif defined(__CC_SOLARIS__)
#error _cc_atomic64_and Unsupported OS
    return 0;
#else
    return __sync_and_and_fetch(a, v);
#endif
}

_CC_API_PUBLIC(int64_t) _cc_atomic64_or(_cc_atomic64_t* a, int64_t v) {
#if defined(__CC_WINDOWS__)
    return (int64_t)InterlockedOr64(a, v);
#elif defined(__CC_MACOSX__) || defined(__CC_IPHONEOS__)
    // return OSAtomicOr32((uint32_t)v, (volatile uint32_t*)a);
    return atomic_fetch_or(a, v);
#elif defined(__CC_SOLARIS__)
#error _cc_atomic64_or Unsupported OS
    return 0;
#else
    return __sync_fetch_and_or(a, v);
#endif
}

_CC_API_PUBLIC(int64_t) _cc_atomic64_xor(_cc_atomic64_t* a, int64_t v) {
#if defined(__CC_WINDOWS__)
    return (int64_t)InterlockedXor64(a, v);
#elif defined(__CC_MACOSX__) || defined(__CC_IPHONEOS__)
    // return OSAtomicXor32((uint32_t)v, (volatile uint32_t*)a);
    return atomic_fetch_xor(a, v);
#elif defined(__CC_SOLARIS__)
#error _cc_atomic64_xor Unsupported OS
    return 0;
#else
    return __sync_fetch_and_xor(a, v);
#endif
}

_CC_API_PUBLIC(bool_t) _cc_atomic64_cas(_cc_atomic64_t* a, int64_t v1, int64_t v2) {
#if defined(__CC_WINDOWS__)
    return (bool_t)(InterlockedCompareExchange64(a, v2, v1) == v1);
#elif defined(__CC_MACOSX__) || defined(__CC_IPHONEOS__)
    // return (bool_t)OSAtomicCompareAndSwap64Barrier(v1, v2, a);
    return (bool_t)atomic_compare_exchange_strong(a, &v1, v2);
#elif defined(__CC_SOLARIS__)
    return (bool_t)(atomic_cas_ptr(a, v1, v2) == v1);
#else
    return (bool_t)__sync_bool_compare_and_swap(a, v1, v2);
#endif
}
