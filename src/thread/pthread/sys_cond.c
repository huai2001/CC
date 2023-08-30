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
#include "sys_thread.c.h"

/* Create a condition variable */
_cc_condition_t *_cc_create_condition(void) {
    _cc_condition_t *cond = (_cc_condition_t *)_cc_malloc(sizeof(_cc_condition_t));
    if (pthread_cond_init(&cond->cond_var, NULL) < 0) {
        _cc_logger_error(_T("pthread_cond_init() failed"));
        _cc_free(cond);
    }
    return (cond);
}

/* Destroy a condition variable */
void _cc_destroy_condition(_cc_condition_t **cond) {
    if (_cc_likely(*cond)) {
        /*pthread_cond_broadcast(&(*cond)->cond);*/
        pthread_cond_destroy(&(*cond)->cond_var);
        _cc_free(*cond);
        *cond = NULL;
    }
}

/* Restart one of the threads that are waiting on the condition variable */
bool_t _cc_condition_signal(_cc_condition_t *cond) {
    if (_cc_unlikely(!cond)) {
        _cc_logger_error(_T("Passed a NULL condition variable"));
        return false;
    }

    if (pthread_cond_signal(&cond->cond_var) != 0) {
        _cc_logger_error(_T("pthread_cond_signal() failed"));
        return false;
    }
    return true;
}

/* Restart all threads that are waiting on the condition variable */
bool_t _cc_condition_broadcast(_cc_condition_t *cond) {
    if (_cc_unlikely(!cond)) {
        _cc_logger_error(_T("Passed a NULL condition variable"));
        return false;
    }

    if (pthread_cond_broadcast(&cond->cond_var) != 0) {
        _cc_logger_error(_T("pthread_cond_broadcast() failed"));
        return false;
    }
    return true;
}

int _cc_condition_wait_timeout(_cc_condition_t *cond, _cc_mutex_t *mutex, uint32_t ms) {
    int retval = 0;
#ifndef _CC_HAVE_CLOCK_GETTIME_
    struct timeval delta;
#endif
    struct timespec abstim;

    if (_cc_unlikely(!cond)) {
        _cc_logger_error(_T("Passed a NULL condition variable"));
        return 0;
    }

#ifdef _CC_HAVE_CLOCK_GETTIME_
    clock_gettime(CLOCK_REALTIME, &abstim);

    abstim.tv_nsec += (ms % 1000) * 1000000;
    abstim.tv_sec += ms / 1000;
#else
    gettimeofday(&delta, NULL);

    abstim.tv_sec = delta.tv_sec + (ms / 1000);
    abstim.tv_nsec = (delta.tv_usec + (ms % 1000) * 1000) * 1000;
#endif
    if (abstim.tv_nsec > 1000000000) {
        abstim.tv_sec += 1;
        abstim.tv_nsec -= 1000000000;
    }
COND_TRY_AGAIN:
    retval = pthread_cond_timedwait(&cond->cond_var, &mutex->ident, &abstim);
    switch (retval) {
    case EINTR:
        goto COND_TRY_AGAIN;
        break;
    case ETIMEDOUT:
        retval = _CC_MUTEX_TIMEDOUT_;
        break;
    case 0:
        break;
    default:
        _cc_logger_error(_T("pthread_cond_timedwait() failed"));
        retval = -1;
        break;
    }
    return retval;
}

/* Wait on the condition variable, unlocking the provided mutex.
 The mutex must be locked before entering this function!
 */
bool_t _cc_condition_wait(_cc_condition_t *cond, _cc_mutex_t *mutex) {
    if (_cc_unlikely(!cond)) {
        _cc_logger_error(_T("Passed a NULL condition variable"));
        return false;
    }

    if (pthread_cond_wait(&cond->cond_var, &mutex->ident) != 0) {
        _cc_logger_error(_T("pthread_cond_wait() failed"));
        return false;
    }
    return true;
}
