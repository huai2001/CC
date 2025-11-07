#include <libcc/atomic.h>

_CC_API_PUBLIC(int32_t) _cc_atomic32_load(_cc_atomic32_t *a) {
#if __CC_STDC_VERSION__ >= 2011
    return (int32_t)atomic_load(a);
#elif defined(__CC_GNUC__)
    return __atomic_load_n(a, __ATOMIC_SEQ_CST);
#else
    int32_t value;
    do {
        value = (int32_t)*a;
    } while (!_cc_atomic32_cas(a, value, value));
    return value;
#endif/*__CC_STDC_VERSION__ >= 2011*/
}

_CC_API_PUBLIC(int64_t) _cc_atomic64_load(_cc_atomic64_t *a) {
#if __CC_STDC_VERSION__ >= 2011
    return (int64_t)atomic_load(a);
#elif defined(__CC_GNUC__)
    return __atomic_load_n(a, __ATOMIC_SEQ_CST);
#else
    int64_t value;
    do {
        value = (int64_t)*a;
    } while (!_cc_atomic64_cas(a, value, value));
    return value;
#endif/*__CC_STDC_VERSION__ >= 2011*/
}

_CC_API_PUBLIC(int32_t) _cc_atomic32_add(_cc_atomic32_t *a, int32_t v) {
#if __CC_STDC_VERSION__ >= 2011
    return atomic_fetch_add(a, v);
#elif defined(__CC_MSVC__)
#if (defined(_M_IA64) || defined(_M_AMD64))
    return (int32_t)InterlockedExchangeAdd(a, v);
#else
    return (int32_t)InterlockedExchangeAdd((long *)a, v);
#endif
#elif defined(__CC_GNUC__)
    return __atomic_fetch_add(a, v, __ATOMIC_SEQ_CST);
#else
    return __sync_fetch_and_add(a, v);
#endif/*__CC_STDC_VERSION__ >= 2011*/
}

_CC_API_PUBLIC(int32_t) _cc_atomic32_sub(_cc_atomic32_t *a, int32_t v) {
#if __CC_STDC_VERSION__ >= 2011
    return atomic_fetch_sub(a, v);
#elif defined(__CC_MSVC__)
#if (defined(_M_IA64) || defined(_M_AMD64))
    return (int32_t)InterlockedExchangeAdd(a, -v);
#else
    return (int32_t)InterlockedExchangeAdd((long *)a, -v);
#endif
#elif defined(__CC_GNUC__)
    return __atomic_fetch_sub(a, v, __ATOMIC_SEQ_CST);
#else
    return __sync_fetch_and_sub(a, v);
#endif/*__CC_STDC_VERSION__ >= 2011*/
}

_CC_API_PUBLIC(int32_t) _cc_atomic32_set(_cc_atomic32_t *a, int32_t v) {
#if __CC_STDC_VERSION__ >= 2011
    return atomic_exchange(a, v);
#elif defined(__CC_MSVC__)
#if (defined(_M_IA64) || defined(_M_AMD64)) && !defined(RC_INVOKED)
    return (int32_t)InterlockedExchange(a, v);
#else
    return (int32_t)InterlockedExchange((long *)a, v);
#endif
#elif defined(__CC_GNUC__)
    return __atomic_exchange_n(a, v, __ATOMIC_SEQ_CST);
#else
    return __sync_lock_test_and_set(a, v);
#endif/*__CC_STDC_VERSION__ >= 2011*/
}

_CC_API_PUBLIC(int32_t) _cc_atomic32_and(_cc_atomic32_t *a, int32_t v) {
#if __CC_STDC_VERSION__ >= 2011
    return atomic_fetch_and(a, v);
#elif defined(__CC_MSVC__)
    return (int32_t)_InterlockedAnd(a, v);
#elif defined(__CC_GNUC__)
    return __atomic_fetch_and(a, v, __ATOMIC_SEQ_CST);
#else
    return __sync_and_and_fetch(a, v);
#endif/*__CC_STDC_VERSION__ >= 2011*/
}

_CC_API_PUBLIC(int32_t) _cc_atomic32_or(_cc_atomic32_t *a, int32_t v) {
#if __CC_STDC_VERSION__ >= 2011
    return atomic_fetch_or(a, v);
#elif defined(__CC_MSVC__)
    return (int32_t)_InterlockedOr(a, v);
#elif defined(__CC_GNUC__)
    return __atomic_fetch_or(a, v, __ATOMIC_SEQ_CST);
#else
    return __sync_fetch_and_or(a, v);
#endif/*__CC_STDC_VERSION__ >= 2011*/
}

_CC_API_PUBLIC(int32_t) _cc_atomic32_xor(_cc_atomic32_t *a, int32_t v) {
#if __CC_STDC_VERSION__ >= 2011
    return atomic_fetch_xor(a, v);
#elif defined(__CC_MSVC__)
    return (int32_t)_InterlockedXor(a, v);
#elif defined(__CC_GNUC__)
    return __atomic_fetch_xor(a, v, __ATOMIC_SEQ_CST);
#else
    return __sync_fetch_and_xor(a, v);
#endif/*__CC_STDC_VERSION__ >= 2011*/
}

_CC_API_PUBLIC(bool_t) _cc_atomic32_cas(_cc_atomic32_t *a, int32_t v1, int32_t v2) {
#if __CC_STDC_VERSION__ >= 2011
    return (bool_t)atomic_compare_exchange_strong(a, &v1, v2);
#elif defined(__CC_MSVC__)
#if (defined(_M_IA64) || defined(_M_AMD64)) && !defined(RC_INVOKED)
    return (bool_t)((int32_t)InterlockedCompareExchange(a, v2, v1) == v1);
#else
    return (bool_t)((int32_t)InterlockedCompareExchange((long *)a, v2, v1) == v1);
#endif
#elif defined(__CC_GNUC__)
    return __atomic_compare_exchange_n(a, &v1, v2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
#else
    return (bool_t)__sync_bool_compare_and_swap(a, v1, v2);
#endif/*__CC_STDC_VERSION__ >= 2011*/
}

_CC_API_PUBLIC(int64_t) _cc_atomic64_add(_cc_atomic64_t *a, int64_t v) {
#if __CC_STDC_VERSION__ >= 2011
    return atomic_fetch_add(a, v);
#elif defined(__CC_MSVC__)
    return (int64_t)InterlockedExchangeAdd64(a, v);
#elif defined(__CC_GNUC__)
    return __atomic_fetch_add(a, v, __ATOMIC_SEQ_CST);
#else
    return __sync_fetch_and_add(a, v);
#endif/*__CC_STDC_VERSION__ >= 2011*/
}

_CC_API_PUBLIC(int64_t) _cc_atomic64_sub(_cc_atomic64_t *a, int64_t v) {
#if __CC_STDC_VERSION__ >= 2011
    return atomic_fetch_sub(a, v);
#elif defined(__CC_MSVC__)
    return (int64_t)InterlockedExchangeAdd64(a, -v);
#elif defined(__CC_GNUC__)
    return __atomic_fetch_sub(a, v, __ATOMIC_SEQ_CST);
#else
    return __sync_fetch_and_sub(a, v);
#endif/*__CC_STDC_VERSION__ >= 2011*/
}

_CC_API_PUBLIC(int64_t) _cc_atomic64_set(_cc_atomic64_t *a, int64_t v) {
#if __CC_STDC_VERSION__ >= 2011
    return atomic_exchange(a, v);
#elif defined(__CC_MSVC__)
    return (int64_t)InterlockedExchange64(a, v);
#elif defined(__CC_GNUC__)
    return __atomic_exchange_n(a, v, __ATOMIC_SEQ_CST);
#else
    return __sync_lock_test_and_set(a, v);
#endif/*__CC_STDC_VERSION__ >= 2011*/
}

_CC_API_PUBLIC(int64_t) _cc_atomic64_and(_cc_atomic64_t *a, int64_t v) {
#if __CC_STDC_VERSION__ >= 2011
    return atomic_fetch_and(a, v);
#elif defined(__CC_MSVC__)
    return (int64_t)InterlockedAnd64(a, v);
#elif defined(__CC_GNUC__)
    return __atomic_fetch_and(a, v, __ATOMIC_SEQ_CST);
#else
    return __sync_and_and_fetch(a, v);
#endif/*__CC_STDC_VERSION__ >= 2011*/
}

_CC_API_PUBLIC(int64_t) _cc_atomic64_or(_cc_atomic64_t *a, int64_t v) {
#if __CC_STDC_VERSION__ >= 2011
    return atomic_fetch_or(a, v);
#elif defined(__CC_MSVC__)
    return (int64_t)InterlockedOr64(a, v);
#elif defined(__CC_GNUC__)
    return __atomic_fetch_or(a, v, __ATOMIC_SEQ_CST);
#else
    return __sync_fetch_and_or(a, v);
#endif/*__CC_STDC_VERSION__ >= 2011*/
}

_CC_API_PUBLIC(int64_t) _cc_atomic64_xor(_cc_atomic64_t *a, int64_t v) {
#if __CC_STDC_VERSION__ >= 2011
    return atomic_fetch_xor(a, v);
#elif defined(__CC_MSVC__)
    return (int64_t)InterlockedXor64(a, v);
#elif defined(__CC_GNUC__)
    return __atomic_fetch_xor(a, v, __ATOMIC_SEQ_CST);
#else
    return __sync_fetch_and_xor(a, v);
#endif/*__CC_STDC_VERSION__ >= 2011*/
}

_CC_API_PUBLIC(bool_t) _cc_atomic64_cas(_cc_atomic64_t *a, int64_t v1, int64_t v2) {
#if __CC_STDC_VERSION__ >= 2011
    return (bool_t)atomic_compare_exchange_strong(a, &v1, v2);
#elif defined(__CC_MSVC__)
    return (bool_t)((int64_t)InterlockedCompareExchange64(a, v2, v1) == v1);
#elif defined(__CC_GNUC__)
    return __atomic_compare_exchange_n(a, &v1, v2, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
#else
    return (bool_t)__sync_bool_compare_and_swap(a, v1, v2);
#endif/*__CC_STDC_VERSION__ >= 2011*/
}
