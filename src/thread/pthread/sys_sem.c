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
#include <libcc/time.h>

#if defined(__CC_IPHONEOS__) || defined(__CC_MACOSX__)
/* Mac OS X doesn't support sem_getvalue() as of version 10.4 */
#include "../apple/sys_sem.c"
#else
#include "sys_thread.c.h"
#include <semaphore.h>

/* Create a semaphore */
_CC_API_PUBLIC(_cc_semaphore_t*) _cc_create_semaphore(int32_t initial_value) {
    /* Allocate sem memory */
    _cc_semaphore_t *sem = (_cc_semaphore_t *)_cc_malloc(sizeof(_cc_semaphore_t));

    if (sem_init(&sem->sem, 0, initial_value) < 0) {
        _cc_logger_error(_T("sem_init() failed"));
        _cc_free(sem);
        sem = nullptr;
    }

    return sem;
}

/* Free the semaphore */
_CC_API_PUBLIC(void) _cc_destroy_semaphore(_cc_semaphore_t **sem) {
    _cc_assert(sem);
    if (_cc_likely(*sem)) {
        sem_destroy(&(*sem)->sem);
        _cc_free((*sem));
        sem = nullptr;
    }
}

_CC_API_PUBLIC(int) _cc_semaphore_wait_timeout(_cc_semaphore_t *sem, uint32_t timeout) {
    int res = -1;
#ifndef _CC_HAVE_CLOCK_GETTIME_
    struct timeval now;
#endif
    struct timespec ts_timeout;
    _cc_assert(sem);

    if (_cc_unlikely(!sem)) {
        _cc_logger_error(_T("Passed a nullptr semaphore"));
        return -1;
    }

    /* Try the easy cases first */
    if (timeout == 0) {
        return _cc_semaphore_try_wait(sem);
    }

    if (timeout == _CC_MUTEX_MAXWAIT_) {
        return _cc_semaphore_wait(sem);
    }

    /* Setup the timeout. sem_timedwait doesn't wait for
     * a lapse of time, but until we reach a certain time.
     * This time is now plus the timeout.
     */
#ifndef _CC_HAVE_CLOCK_GETTIME_
    gettimeofday(&now, nullptr);

    /* Add our timeout to current time */
    ts_timeout.tv_sec = now.tv_sec + (timeout / 1000);
    ts_timeout.tv_nsec = (now.tv_usec + (timeout % 1000) * 1000) * 1000;
#else
    clock_gettime(CLOCK_REALTIME, &ts_timeout);

    /* Add our timeout to current time */
    ts_timeout.tv_nsec += (timeout % 1000) * 1000000;
    ts_timeout.tv_sec += timeout / 1000;
#endif

    /* Wrap the second if needed */
    if (ts_timeout.tv_nsec > 1000000000) {
        ts_timeout.tv_sec += 1;
        ts_timeout.tv_nsec -= 1000000000;
    }

    /* Wait. */
    do
        res = sem_timedwait(&sem->sem, &ts_timeout);
    while (res < 0 && errno == EINTR);

    if (res < 0) {
        if (errno == ETIMEDOUT) {
            res = _CC_MUTEX_TIMEDOUT_;
        } else {
            _cc_logger_error(_T("sem_timedwait returned an error: %s"), strerror(errno));
        }
    }

    return res;
}

_CC_API_PUBLIC(int) _cc_semaphore_try_wait(_cc_semaphore_t *sem) {
    int res;
    _cc_assert(sem);

    if (_cc_unlikely(!sem)) {
        _cc_logger_error(_T("Passed a nullptr semaphore"));
        return -1;
    }

    res = _CC_MUTEX_TIMEDOUT_;
    if (sem_trywait(&sem->sem) == 0) {
        return 0;
    }

    return res;
}

_CC_API_PUBLIC(int) _cc_semaphore_wait(_cc_semaphore_t *sem) {
    int res;
    _cc_assert(sem);

    if (_cc_unlikely(!sem)) {
        _cc_logger_error(_T("Passed a nullptr semaphore"));
        return -1;
    }

    res = sem_wait(&sem->sem);
    if (res < 0) {
        _cc_logger_error(_T("sem_wait() failed"));
    }

    return res;
}

/* Returns the current count of the semaphore */
_CC_API_PUBLIC(uint32_t) _cc_semaphore_value(_cc_semaphore_t *sem) {
    int res = 0;
    _cc_assert(sem);

    sem_getvalue(&sem->sem, &res);
    if (res < 0) {
        res = 0;
    }

    return (uint32_t)res;
}

_CC_API_PUBLIC(bool_t) _cc_semaphore_post(_cc_semaphore_t *sem) {
    _cc_assert(sem);

    if (_cc_unlikely(!sem)) {
        _cc_logger_error(_T("Passed a nullptr semaphore"));
        return false;
    }

    if (sem_post(&sem->sem) < 0) {
        _cc_logger_error(_T("sem_post() failed"));
        return false;
    }

    return true;
}

#endif /* __CC_MACOSX__ */
