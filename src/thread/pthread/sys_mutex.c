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

/** Create a mutex, initialized unlocked */
_cc_mutex_t *_cc_create_mutex(void) {
    _cc_mutex_t *mutex;
    pthread_mutexattr_t attr;

    /* Allocate mutex memory */
    mutex = (_cc_mutex_t *)_cc_malloc(sizeof(_cc_mutex_t));
    bzero(mutex, sizeof(_cc_mutex_t));
    /* Create the mutex, with initial value signaled */
    pthread_mutexattr_init(&attr);
#if _CC_THREAD_PTHREAD_RECURSIVE_MUTEX_
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
#elif _CC_THREAD_PTHREAD_RECURSIVE_MUTEX_NP_
    pthread_mutexattr_setkind_np(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
#endif

    if (pthread_mutex_init(&mutex->ident, &attr) != 0) {
        _cc_logger_error(_T("Couldn't create mutex"));
        _cc_free(mutex);
        mutex = NULL;
    }

    return (mutex);
}

/* Free the mutex */
void _cc_destroy_mutex(_cc_mutex_t **mutex) {
    if (_cc_likely(*mutex)) {
        pthread_mutex_destroy(&(*mutex)->ident);
        _cc_free(*mutex);
        *mutex = NULL;
    }
}

/* Lock the mutex */
bool_t _cc_mutex_lock(_cc_mutex_t *mutex) {
#if _CC_FAKE_RECURSIVE_MUTEX_
    pthread_t self;
#endif
    if (_cc_unlikely(!mutex)) {
        _cc_logger_error(_T("Passed a NULL mutex"));
        return false;
    }

#if _CC_FAKE_RECURSIVE_MUTEX_
    self = pthread_self();
    if (mutex->owner == self) {
        ++mutex->recursive;
    } else {
        /* The order of operations is important.
           We set the locking thread id after we obtain the lock
           so unlocks from other threads will fail.
         */
        if (pthread_mutex_lock(&mutex->ident) == 0) {
            mutex->owner = self;
            mutex->recursive = 0;
        } else {
            _cc_logger_error(_T("pthread_mutex_lock() failed"));
            return false;
        }
    }
#else
    if (pthread_mutex_lock(&mutex->ident) < 0) {
        _cc_logger_error(_T("pthread_mutex_lock() failed"));
        return false;
    }
#endif
    return true;
}

/* Try Lock the mutex */
int _cc_mutex_try_lock(_cc_mutex_t *mutex) {
#if _CC_FAKE_RECURSIVE_MUTEX_
    pthread_t self;
#endif
    if (_cc_unlikely(!mutex)) {
        _cc_logger_error(_T("Passed a NULL mutex"));
        return -1;
    }
#if _CC_FAKE_RECURSIVE_MUTEX_
    self = pthread_self();
    if (mutex->owner == self) {
        ++mutex->recursive;
    } else {
        /* The order of operations is important.
         We set the locking thread id after we obtain the lock
         so unlocks from other threads will fail.
         */
        if (pthread_mutex_trylock(&mutex->ident) == 0) {
            mutex->owner = self;
            mutex->recursive = 0;
        } else if (errno == EBUSY) {
            return 1;
        } else {
            _cc_logger_error(_T("pthread_mutex_trylock() failed"));
            return -1;
        }
    }
#else
    if (pthread_mutex_trylock(&mutex->ident) != 0) {
        if (errno == EBUSY) {
            return _CC_MUTEX_TIMEDOUT_;
        } else {
            _cc_logger_error(_T("pthread_mutex_trylock() failed"));
            return -1;
        }
    }
#endif
    return 0;
}

/* Unlock the mutex */
bool_t _cc_mutex_unlock(_cc_mutex_t *mutex) {
    if (_cc_unlikely(!mutex)) {
        _cc_logger_error(_T("Passed a NULL mutex"));
        return false;
    }
#if _CC_FAKE_RECURSIVE_MUTEX_
    /* We can only unlock the mutex if we own it */
    if (pthread_self() == mutex->owner) {
        if (mutex->recursive) {
            --mutex->recursive;
        } else {
            /* The order of operations is important.
               First reset the owner so another thread doesn't lock
               the mutex and set the ownership before we reset it,
               then release the lock semaphore.
             */
            mutex->owner = 0;
            pthread_mutex_unlock(&mutex->ident);
        }
    } else {
        _cc_logger_error(_T("mutex not owned by this thread"));
        return false;
    }

#else
    if (pthread_mutex_unlock(&mutex->ident) < 0) {
        _cc_logger_error(_T("pthread_mutex_unlock() failed"));
        return false;
    }
#endif
    return true;
}
