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
#include <cc/mutex.h>
#include <cc/time.h>

#ifdef __CC_WINDOWS__
#include <cc/core/windows.h>
#endif

#if defined(__CC_SOLARIS__)
#include <atomic.h>
#endif

_CC_API_PUBLIC(void) _cc_spin_lock_init(_cc_spinlock_t *lock) {
    if (_cc_cpu_number_processors <= 0) {
        _cc_cpu_count();
    }
    *lock = 0;
}

/**/
_CC_API_PUBLIC(void) _cc_spin_lock(_cc_spinlock_t *lock) {
    _cc_lock((_cc_atomic32_t *)lock, 1, _CC_LOCK_SPIN_);
}

/**/
_CC_API_PUBLIC(void) _cc_spin_unlock(_cc_rwlock_t *lock) {
    *lock = 0;
}