/*
 * Copyright libcc.cn@gmail.com. and other libCC contributors.
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

/* Create a semaphore */
_CC_API_PUBLIC(_cc_semaphore_t*) _cc_create_semaphore(int32_t initial_value) {
    /* Allocate sem memory */
    _cc_semaphore_t *sem = (_cc_semaphore_t *)_cc_malloc(sizeof(_cc_semaphore_t));
    sem->sem = dispatch_semaphore_create(initial_value);
    if (sem->sem == nullptr) {
        _cc_logger_error(_T("create semaphore failed"));
        _cc_destroy_semaphore(&sem);
        return nullptr;
    }

    return sem;
}

/* Free the semaphore */
_CC_API_PUBLIC(void) _cc_destroy_semaphore(_cc_semaphore_t **sem) {
    if (*sem) {
        _cc_safe_free((*sem));
    }
}

_CC_API_PUBLIC(int) _cc_semaphore_wait_timeout(_cc_semaphore_t *sem, uint32_t timeout) {
    dispatch_time_t t = 0;
    if (!sem) {
        _cc_logger_error(_T("Passed a nullptr semaphore"));
        return -1;
    }

    /* A timeout of 0 is an easy case */
    if (timeout == 0) {
        return (int)dispatch_semaphore_wait(sem->sem, DISPATCH_TIME_FOREVER);
    }

    t = dispatch_time(DISPATCH_TIME_NOW, timeout * NSEC_PER_MSEC);
    return (int)dispatch_semaphore_wait(sem->sem, t);
}

_CC_API_PUBLIC(int) _cc_semaphore_try_wait(_cc_semaphore_t *sem) {
    int res = _CC_MUTEX_TIMEDOUT_;
    if (!sem) {
        _cc_logger_error(_T("Passed a nullptr semaphore"));
        return -1;
    }

    return (uint32_t)res;
}

_CC_API_PUBLIC(int) _cc_semaphore_wait(_cc_semaphore_t *sem) {
    if (!sem) {
        _cc_logger_error(_T("Passed a nullptr semaphore"));
        return -1;
    }
    return (int)dispatch_semaphore_wait(sem->sem, DISPATCH_TIME_FOREVER);
}

/* Returns the current count of the semaphore */
_CC_API_PUBLIC(uint32_t) _cc_semaphore_value(_cc_semaphore_t *sem) {
    int res = 0;
    return (uint32_t)res;
}

_CC_API_PUBLIC(bool_t) _cc_semaphore_post(_cc_semaphore_t *sem) {
    if (!sem) {
        _cc_logger_error(_T("Passed a nullptr semaphore"));
        return false;
    }

    dispatch_semaphore_signal(sem->sem);

    return true;
}
