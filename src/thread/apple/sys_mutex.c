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
#include "sys_thread.c.h"

/** Create a mutex, initialized unlocked */
_CC_API_PUBLIC(_cc_mutex_t*) _cc_alloc_mutex(void) {
    /* Allocate mutex memory */
    _cc_mutex_t *mutex = (_cc_mutex_t *)_cc_malloc(sizeof(_cc_mutex_t));
    bzero(mutex, sizeof(_cc_mutex_t));
    mutex->unfair_lock = OS_UNFAIR_LOCK_INIT;
    
    return mutex;
}

/* Free the mutex */
_CC_API_PUBLIC(void) _cc_free_mutex(_cc_mutex_t *mutex) {
    if (_cc_likely(mutex)) {
        _cc_free(mutex);
    }
}

/* Lock the mutex */
_CC_API_PUBLIC(bool_t) _cc_mutex_lock(_cc_mutex_t *mutex) {
    pthread_t self;
    if (_cc_unlikely(mutex == nullptr)) {
        _cc_logger_error(_T("Passed a nullptr mutex"));
        return false;
    }

    self = pthread_self();
    if (mutex->owner == self) {
        ++mutex->recursive;
    } else {
        os_unfair_lock_lock(&(mutex->unfair_lock));
        mutex->owner = self;
        mutex->recursive = 0;
    }
    return true;
}

/* Try Lock the mutex */
_CC_API_PUBLIC(int) _cc_mutex_try_lock(_cc_mutex_t *mutex) {
    pthread_t self;
    if (_cc_unlikely(mutex == nullptr)) {
        _cc_logger_error(_T("Passed a nullptr mutex"));
        return -1;
    }
    self = pthread_self();
    if (mutex->owner == self) {
        ++mutex->recursive;
        return 1;
    } else {
        if (os_unfair_lock_trylock(&(mutex->unfair_lock))) {
            mutex->owner = self;
            mutex->recursive = 0;
            return 1;
        }
    }
    return 0;
}

/* Unlock the mutex */
_CC_API_PUBLIC(bool_t) _cc_mutex_unlock(_cc_mutex_t *mutex) {
    if (_cc_unlikely(mutex == nullptr)) {
        _cc_logger_error(_T("Passed a nullptr mutex"));
        return false;
    }
    /* We can only unlock the mutex if we own it */
    if (pthread_self() == mutex->owner) {
        if (mutex->recursive) {
            --mutex->recursive;
        } else {
            mutex->owner = nullptr;
            mutex->recursive = 0;
            os_unfair_lock_unlock(&(mutex->unfair_lock));
        }
    }
    return true;
}
