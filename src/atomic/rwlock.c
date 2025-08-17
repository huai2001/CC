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

#include <libcc/atomic.h>
#ifndef __CC_WINDOWS__
#include <pthread.h>
#endif

#if defined(__CC_SOLARIS__)
#include <atomic.h>
#endif

#define _CC_RWLOCK_WLOCK_ (-1)

_CC_API_PUBLIC(void) _cc_cpu_pause() {
#if __CC_WINDOWS__
#if defined(__BORLANDC__) || (__WATCOMC__ < 1230)
/*
 * Borland C++ 5.5 (tasm32) and Open Watcom C prior to 1.3
 * do not understand the "pause" instruction
 */
#else
    __asm { pause }
#endif
#else
#if (__i386__ || __i386 || __amd64__ || __amd64)
    __asm__ __volatile__("pause");
#elif __aarch64__
    __asm__ __volatile__("yield");
#endif
#endif
}

/**/
_CC_API_PUBLIC(void) _cc_lock_init(_cc_atomic_lock_t *lock) {
    if (_cc_cpu_number_processors <= 0) {
        _cc_cpu_count();
    }
    *lock = 0;
}

/**/
_CC_API_PUBLIC(void) _cc_lock(_cc_atomic_lock_t *lock, uint32_t value, uint32_t spin) {
    uint32_t i, n;
    for (;;) {
        if (*lock == 0 && _cc_atomic32_cas(lock, 0, value)) {
            return;
        }

        if (_cc_cpu_number_processors > 0) {
            for (n = 1; n < spin; n <<= 1) {
                for (i = 0; i < n; i++) {
                    _cc_cpu_pause();
                }
                if (*lock == 0 && _cc_atomic32_cas(lock, 0, value)) {
                    return;
                }
            }
        }
#ifdef __CC_WINDOWS__
        SwitchToThread();
#else
        sched_yield();
        //_cc_nsleep(1);
#endif
    }
}

/**/
_CC_API_PUBLIC(void) _cc_rwlock_rlock(_cc_atomic_lock_t *lock) {
    int32_t i, n;
    _cc_atomic32_t readers;

    for (;;) {
        readers = *lock;
        if (readers != _CC_RWLOCK_WLOCK_ && _cc_atomic32_cas(lock, readers, readers + 1)) {
            return;
        }

        if (_cc_cpu_number_processors > 1) {
            for (n = 1; n < _CC_LOCK_SPIN_; n <<= 1) {
                for (i = 0; i < n; i++) {
                    _cc_cpu_pause();
                }
                readers = *lock;
                if (readers != _CC_RWLOCK_WLOCK_ && _cc_atomic32_cas(lock, readers, readers + 1)) {
                    return;
                }
            }
        }
#ifdef __CC_WINDOWS__
        SwitchToThread();
#else
        sched_yield();
#endif
    }
}

/**/
_CC_API_PUBLIC(void) _cc_rwlock_wlock(_cc_atomic_lock_t *lock) {
    _cc_lock((_cc_atomic32_t *)lock, _CC_RWLOCK_WLOCK_, _CC_LOCK_SPIN_);
}

/**/
_CC_API_PUBLIC(void) _cc_rwlock_unlock(_cc_atomic_lock_t *lock) {
    if (*lock == _CC_RWLOCK_WLOCK_) {
        _cc_atomic32_cas(lock, _CC_RWLOCK_WLOCK_, 0);
    } else {
        _cc_atomic32_dec(lock);
    }
}

/**/
_CC_API_PUBLIC(void) _cc_rwlock_downgrade(_cc_atomic_lock_t *lock) {
    if (*lock == _CC_RWLOCK_WLOCK_) {
        *lock = 1;
    }
}
