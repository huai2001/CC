#ifndef _C_CC_ATOMIC_H_INCLUDED_
#define _C_CC_ATOMIC_H_INCLUDED_

#include "os.h"
#include <libcc/time.h>

#if __CC_STDC_VERSION__ >= 2011
#include <stdatomic.h>
#endif

#if defined(__CC_WINDOWS__)
    #include <libcc/os/windows.h>
    /* Need to do this here because intrin.h has C++ code in it */
    /* Visual Studio 2005 has a bug where intrin.h conflicts with winnt.h */
    #if defined(__CC_MSVC__) && (__CC_MSVC__ >= 1500)
    #include <intrin.h>
    #endif
#elif defined(__CC_MACOSX__) || defined(__CC_IPHONEOS__)
    #include <libkern/OSAtomic.h>
    #include <stdatomic.h>
#elif defined(__CC_SOLARIS__)
    #include <atomic.h>
#endif

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#if __CC_STDC_VERSION__ >= 2011
    typedef volatile _Atomic(int32_t) _cc_atomic32_t;
    typedef volatile _Atomic(int64_t) _cc_atomic64_t;
#elif defined(__CC_GNUC__)
    typedef volatile int_least32_t _cc_atomic32_t;
    typedef volatile int_least64_t _cc_atomic64_t;
#elif defined(__CC_WINDOWS__)
    typedef volatile long _cc_atomic32_t;
    typedef volatile __int64 _cc_atomic64_t;
#else
    typedef volatile int _cc_atomic32_t;
    typedef volatile long long int _cc_atomic64_t;
#endif/*__CC_STDC_VERSION__ >= 2011*/
/*/////////////////////////////////////////////////////////////////////////*/
#define _cc_atomic32_inc(a) _cc_atomic32_add(a, 1)
#define _cc_atomic32_dec(a) _cc_atomic32_sub(a, 1)

_CC_API_PUBLIC(int32_t) _cc_atomic32_add(_cc_atomic32_t*,int32_t);
_CC_API_PUBLIC(int32_t) _cc_atomic32_sub(_cc_atomic32_t*,int32_t);
_CC_API_PUBLIC(int32_t) _cc_atomic32_set(_cc_atomic32_t*,int32_t);
_CC_API_PUBLIC(int32_t) _cc_atomic32_load(_cc_atomic32_t *a);
_CC_API_PUBLIC(int32_t) _cc_atomic32_and(_cc_atomic32_t*,int32_t);
_CC_API_PUBLIC(int32_t) _cc_atomic32_or(_cc_atomic32_t*,int32_t);
_CC_API_PUBLIC(int32_t) _cc_atomic32_xor(_cc_atomic32_t*,int32_t);
_CC_API_PUBLIC(bool_t) _cc_atomic32_cas(_cc_atomic32_t*,int32_t,int32_t);

#define _cc_atomic64_inc(a) _cc_atomic64_add(a, 1)
#define _cc_atomic64_dec(a) _cc_atomic64_sub(a, 1)
_CC_API_PUBLIC(int64_t) _cc_atomic64_add(_cc_atomic64_t*,int64_t);
_CC_API_PUBLIC(int64_t) _cc_atomic64_sub(_cc_atomic64_t*,int64_t);
_CC_API_PUBLIC(int64_t) _cc_atomic64_set(_cc_atomic64_t*,int64_t);
_CC_API_PUBLIC(int64_t) _cc_atomic64_load(_cc_atomic64_t *a);
_CC_API_PUBLIC(int64_t) _cc_atomic64_and(_cc_atomic64_t*,int64_t);
_CC_API_PUBLIC(int64_t) _cc_atomic64_or(_cc_atomic64_t*,int64_t);
_CC_API_PUBLIC(int64_t) _cc_atomic64_xor(_cc_atomic64_t*,int64_t);
_CC_API_PUBLIC(bool_t) _cc_atomic64_cas(_cc_atomic64_t*,int64_t,int64_t);

/**
 * @brief Increment an atomic variable used as a reference count.
 */
#define _cc_atomic32_inc_ref(a) (_cc_atomic32_inc(a) == 0)
#define _cc_atomic64_inc_ref(a) (_cc_atomic64_inc(a) == 0)
/**
 * @brief Decrement an atomic variable used as a reference count.
 *
 * @return true if the variable reached zero after decrementing, false otherwise
 */
#define _cc_atomic32_dec_ref(a) (_cc_atomic32_dec(a) == 1)
#define _cc_atomic64_dec_ref(a) (_cc_atomic64_dec(a) == 1)

#ifdef __CC_CPU64__
    typedef _cc_atomic64_t _cc_atomic_t;
    #define _cc_atomic_inc   _cc_atomic64_inc
    #define _cc_atomic_dec   _cc_atomic64_dec
    #define _cc_atomic_add   _cc_atomic64_add
    #define _cc_atomic_sub   _cc_atomic64_sub
    #define _cc_atomic_set   _cc_atomic64_set
    #define _cc_atomic_cas   _cc_atomic64_cas

    #define _cc_atomic_inc_ref   _cc_atomic64_inc_ref
    #define _cc_atomic_dec_ref   _cc_atomic64_dec_ref
#else
    typedef _cc_atomic32_t _cc_atomic_t;
    #define _cc_atomic_inc   _cc_atomic32_inc
    #define _cc_atomic_dec   _cc_atomic32_dec
    #define _cc_atomic_add   _cc_atomic32_add
    #define _cc_atomic_sub   _cc_atomic32_sub
    #define _cc_atomic_set   _cc_atomic32_set
    #define _cc_atomic_cas   _cc_atomic32_cas

    #define _cc_atomic_inc_ref   _cc_atomic32_inc_ref
    #define _cc_atomic_dec_ref   _cc_atomic32_dec_ref
#endif

typedef _cc_atomic32_t  _cc_atomic_lock_t;
typedef _cc_atomic32_t  _cc_rwlock_t;

#define _CC_LOCK_SPIN_  2048

/**/
_CC_API_PUBLIC(void) _cc_lock_init(_cc_atomic_lock_t *lock);
/**/
_CC_API_PUBLIC(void) _cc_lock(_cc_atomic_lock_t *lock, uint32_t value, uint32_t spin);

/**/
_CC_FORCE_INLINE_ bool_t _cc_try_lock(_cc_atomic_lock_t *lock) {
    return (*lock == 0 && _cc_atomic32_cas(lock, 0, 1));
}

/**/
_CC_FORCE_INLINE_ void _cc_unlock(_cc_atomic_lock_t *lock) {
    //_cc_atomic32_set(lock, 0);
    *lock = 0;
}

/**/
_CC_FORCE_INLINE_ void _cc_spin_lock(_cc_atomic_lock_t *lock) {
    _cc_lock(lock, 1, _CC_LOCK_SPIN_);
}

/**/
_CC_API_PUBLIC(void) _cc_rwlock_rlock(_cc_atomic_lock_t *lock);
/**/
_CC_API_PUBLIC(void) _cc_rwlock_wlock(_cc_atomic_lock_t *lock);
/**/
_CC_API_PUBLIC(void) _cc_rwlock_unlock(_cc_atomic_lock_t *lock);
/**/
_CC_API_PUBLIC(void) _cc_rwlock_downgrade(_cc_atomic_lock_t *lock);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_ATOMIC_H_INCLUDED_*/
