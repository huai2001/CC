#include "sys_thread.c.h"

/* Create a semaphore */
_CC_API_PUBLIC(_cc_semaphore_t*) _cc_alloc_semaphore(int32_t initial_value) {
    /* Allocate sem memory */
    _cc_semaphore_t *sem = (_cc_semaphore_t *)_cc_malloc(sizeof(_cc_semaphore_t));
    sem->sem = dispatch_semaphore_create(initial_value);
    if (sem->sem == NULL) {
        _cc_logger(_CC_LOG_LEVEL_ERROR_, "Create semaphore failed");
        _cc_free_semaphore(sem);
        return NULL;
    }

    return sem;
}

/* Free the semaphore */
_CC_API_PUBLIC(void) _cc_free_semaphore(_cc_semaphore_t *sem) {
    if (sem) {
        _cc_free(sem);
    }
}

_CC_API_PUBLIC(int) _cc_semaphore_wait_timeout(_cc_semaphore_t *sem, uint32_t timeout) {
    dispatch_time_t t = 0;
    if (!sem) {
        _cc_logger(_CC_LOG_LEVEL_ERROR_, "Passed a NULL semaphore");
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
        _cc_logger(_CC_LOG_LEVEL_ERROR_, "Passed a NULL semaphore");
        return -1;
    }

    return (uint32_t)res;
}

_CC_API_PUBLIC(int) _cc_semaphore_wait(_cc_semaphore_t *sem) {
    if (!sem) {
        _cc_logger(_CC_LOG_LEVEL_ERROR_, "Passed a NULL semaphore");
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
        _cc_logger(_CC_LOG_LEVEL_ERROR_, "Passed a NULL semaphore");
        return false;
    }

    dispatch_semaphore_signal(sem->sem);

    return true;
}
