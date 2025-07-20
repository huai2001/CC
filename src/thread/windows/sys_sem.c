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

#if defined(__CC_WIN32_CE__) && (__CC_WIN32_CE__ < 300)
#define _CC_CreateSemaphore(A, B, C, D) CreateSemaphoreCE(A, B, C, D)
#define _CC_CloseHandle CloseSynchHandle
#define _CC_WaitForSingleObject(A, B) WaitForSemaphoreCE(A, B)
#define _CC_ReleaseSemaphore ReleaseSemaphoreCE
#elif __WINRT__
#define _CC_CreateSemaphore(A, B, C, D) CreateSemaphoreEx(A, B, C, D, 0, SEMAPHORE_ALL_ACCESS)
#define _CC_CloseHandle CloseSynchHandle
#define _CC_WaitForSingleObject(A, B) WaitForSingleObjectEx(A, B, false)
#define _CC_ReleaseSemaphore ReleaseSemaphoreCE
#else
#define _CC_CreateSemaphore(A, B, C, D) CreateSemaphore(A, B, C, D)
#define _CC_CloseHandle CloseHandle
#define _CC_WaitForSingleObject(A, B) WaitForSingleObject(A, B)
#define _CC_ReleaseSemaphore ReleaseSemaphore
#endif

/* Create a semaphore */
_CC_API_PUBLIC(_cc_semaphore_t*) _cc_create_semaphore(int32_t initial_value) {
    /* Allocate sem memory */
    _cc_semaphore_t *sem = _CC_MALLOC(_cc_semaphore_t);
    /* Create the semaphore, with max value 32K */
    sem->ident = _CC_CreateSemaphore(nullptr, initial_value, 32 * 1024, nullptr);
    sem->count = (LONG)initial_value;
    if (!sem->ident) {
        _cc_logger_error(_T("Couldn't create semaphore"));
        _cc_free(sem);
        sem = nullptr;
    }
    return (sem);
}

/* Free the semaphore */
_CC_API_PUBLIC(void) _cc_destroy_semaphore(_cc_semaphore_t **sem) {
    if (sem && *sem) {
        if ((*sem)->ident) {
            _CC_CloseHandle((*sem)->ident);
            (*sem)->ident = 0;
        }
        _cc_free((*sem));
        *sem = nullptr;
    }
}

/**/
_CC_API_PUBLIC(int) _cc_semaphore_wait_timeout(_cc_semaphore_t *sem, uint32_t timeout) {
    bool_t retval = 0;
    DWORD dwMilliseconds;

    if (!sem) {
        _cc_logger_error(_T("Passed a nullptr sem"));
        return false;
    }

    if (timeout == _CC_MUTEX_MAXWAIT_) {
        dwMilliseconds = INFINITE;
    } else {
        dwMilliseconds = (DWORD)timeout;
    }

    switch (_CC_WaitForSingleObject(sem->ident, dwMilliseconds)) {
    case WAIT_OBJECT_0:
        InterlockedDecrement(&sem->count);
        retval = 0;
        break;
    case WAIT_TIMEOUT:
        retval = _CC_MUTEX_TIMEDOUT_;
        break;
    default:
        _cc_logger_error(_T("WaitForSingleObject() failed"));
        retval = 0;
        break;
    }
    return retval;
}

/**/
_CC_API_PUBLIC(int) _cc_semaphore_try_wait(_cc_semaphore_t *sem) {
    return _cc_semaphore_wait_timeout(sem, 0);
}

/**/
_CC_API_PUBLIC(int) _cc_semaphore_wait(_cc_semaphore_t *sem) {
    return _cc_semaphore_wait_timeout(sem, _CC_MUTEX_MAXWAIT_);
}

/* Returns the current count of the semaphore */
_CC_API_PUBLIC(uint32_t) _cc_semaphore_value(_cc_semaphore_t *sem) {
    if (!sem) {
        _cc_logger_error(_T("Passed a nullptr sem"));
        return 0;
    }
    return (uint32_t)sem->count;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_semaphore_post(_cc_semaphore_t *sem) {
    if (!sem) {
        _cc_logger_error(_T("Passed a nullptr sem"));
        return false;
    }
    /* Increase the counter in the first place, because
     * after a successful release the semaphore may
     * immediately get destroyed by another thread which
     * is waiting for this semaphore.
     */
    InterlockedIncrement(&sem->count);
    if (_CC_ReleaseSemaphore(sem->ident, 1, nullptr) == false) {
        /* restore */
        InterlockedDecrement(&sem->count);
        _cc_logger_error(_T("ReleaseSemaphore() failed"));
        return false;
    }
    return true;
}
