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
    if (_cc_unlikely(mutex == NULL)) {
        _cc_logger(_CC_LOG_LEVEL_ERROR_, "Passed a NULL mutex");
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
    if (_cc_unlikely(mutex == NULL)) {
        _cc_logger(_CC_LOG_LEVEL_ERROR_, "Passed a NULL mutex");
        return -1;
    }
    self = pthread_self();
    if (mutex->owner == self) {
        ++mutex->recursive;
    } else {
        if (os_unfair_lock_trylock(&(mutex->unfair_lock))) {
            mutex->owner = self;
            mutex->recursive = 0;
        } else {
            return _CC_MUTEX_TIMEDOUT_;
        }
    }
    return 0;
}

/* Unlock the mutex */
_CC_API_PUBLIC(bool_t) _cc_mutex_unlock(_cc_mutex_t *mutex) {
    if (_cc_unlikely(mutex == NULL)) {
        _cc_logger(_CC_LOG_LEVEL_ERROR_, "Passed a NULL mutex");
        return false;
    }
    /* We can only unlock the mutex if we own it */
    if (pthread_self() == mutex->owner) {
        if (mutex->recursive) {
            --mutex->recursive;
        } else {
            mutex->owner = NULL;
            mutex->recursive = 0;
            os_unfair_lock_unlock(&(mutex->unfair_lock));
        }
    }
    return true;
}
