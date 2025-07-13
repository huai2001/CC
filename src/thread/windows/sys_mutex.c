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

/**
 * Implementation based on Slim Reader/Writer (SRW) Locks for Win 7 and newer.
 */
#if _CC_WINDOWS_FORCE_MUTEX_CRITICAL_SECTIONS_ == 0
#if __WINRT__
/* Functions are guaranteed to be available */
#define pInitializeSRWLock InitializeSRWLock
#define pReleaseSRWLockExclusive ReleaseSRWLockExclusive
#define pAcquireSRWLockExclusive AcquireSRWLockExclusive
#define pTryAcquireSRWLockExclusive TryAcquireSRWLockExclusive
#else
typedef VOID(WINAPI *pfnInitializeSRWLock)(PSRWLOCK);
typedef VOID(WINAPI *pfnReleaseSRWLockExclusive)(PSRWLOCK);
typedef VOID(WINAPI *pfnAcquireSRWLockExclusive)(PSRWLOCK);
typedef BOOLEAN(WINAPI *pfnTryAcquireSRWLockExclusive)(PSRWLOCK);
static pfnInitializeSRWLock pInitializeSRWLock = nullptr;
static pfnReleaseSRWLockExclusive pReleaseSRWLockExclusive = nullptr;
static pfnAcquireSRWLockExclusive pAcquireSRWLockExclusive = nullptr;
static pfnTryAcquireSRWLockExclusive pTryAcquireSRWLockExclusive = nullptr;
#endif
#endif

/** Create a mutex, initialized unlocked */
_CC_API_PRIVATE(_cc_mutex_t*) _cc_create_mutex_cs(void) {
    /* Allocate mutex memory */
    struct _cc_mutex_cs *mutex = _CC_MALLOC(struct _cc_mutex_cs);
    bzero(mutex, sizeof(struct _cc_mutex_cs));
/* On SMP systems, a non-zero spin count generally helps performance */
#if __CC_WINRT__
    InitializeCriticalSectionEx(&mutex->cs, 2000, 0);
#else
    InitializeCriticalSectionAndSpinCount(&mutex->cs, 2000);
#endif

    return (_cc_mutex_t *)mutex;
}

/** Create a mutex, initialized unlocked */
_CC_API_PRIVATE(_cc_mutex_t*) _cc_create_mutex_srw(void) {
    /* Allocate mutex memory */
    struct _cc_mutex_srw *mutex = _CC_MALLOC(struct _cc_mutex_srw);
    if (mutex) {
        pInitializeSRWLock(&mutex->srw);
        bzero(mutex, sizeof(struct _cc_mutex_srw));
    }
    return (_cc_mutex_t *)mutex;
}

/* Free the mutex */
_CC_API_PRIVATE(void) _cc_destroy_mutex_cs(_cc_mutex_t *mutex_cs) {
    struct _cc_mutex_cs *mutex = (struct _cc_mutex_cs *)mutex_cs;
    DeleteCriticalSection(&mutex->cs);
    _cc_free(mutex_cs);
}

/* Free the mutex */
_CC_API_PRIVATE(void) _cc_destroy_mutex_srw(_cc_mutex_t *mutex_srw) {
    _cc_free(mutex_srw);
}

/* Lock the mutex */
_CC_API_PRIVATE(bool_t) _cc_mutex_lock_cs(_cc_mutex_t *mutex_cs) {
    struct _cc_mutex_cs *mutex = (struct _cc_mutex_cs *)mutex_cs;
    EnterCriticalSection(&mutex->cs);
    return true;
}

/* Lock the mutex */
_CC_API_PRIVATE(bool_t) _cc_mutex_lock_srw(_cc_mutex_t *mutex_srw) {
    struct _cc_mutex_srw *mutex = (struct _cc_mutex_srw *)mutex_srw;
    DWORD self = GetCurrentThreadId();
    if (mutex->owner == self) {
        ++mutex->count;
    } else {
        /* The order of operations is important.
           We set the locking thread id after we obtain the lock
           so unlocks from other threads will fail.
         */
        pAcquireSRWLockExclusive(&mutex->srw);
        _cc_assert(mutex->count == 0 && mutex->owner == 0);
        mutex->owner = self;
        mutex->count = 1;
    }
    return true;
}

/* try lock the mutex */
_CC_API_PRIVATE(int) _cc_mutex_try_lock_cs(_cc_mutex_t *mutex_cs) {
    struct _cc_mutex_cs *mutex = (struct _cc_mutex_cs *)mutex_cs;
    if (TryEnterCriticalSection(&mutex->cs) == 0) {
        return _CC_MUTEX_TIMEDOUT_;
    }
    return 0;
}

/* try lock the mutex */
_CC_API_PRIVATE(int) _cc_mutex_try_lock_srw(_cc_mutex_t *mutex_srw) {
    struct _cc_mutex_srw *mutex = (struct _cc_mutex_srw *)mutex_srw;
    DWORD self;

    if (mutex == nullptr) {
        _cc_logger_error(_T("Passed a nullptr mutex"));
        return -1;
    }

    self = GetCurrentThreadId();
    if (mutex->owner == self) {
        ++mutex->count;
    } else {
        if (pTryAcquireSRWLockExclusive(&mutex->srw) != 0) {
            _cc_assert(mutex->count == 0 && mutex->owner == 0);
            mutex->owner = self;
            mutex->count = 1;
        } else {
            return _CC_MUTEX_TIMEDOUT_;
        }
    }
    return 0;
}

/**/
_CC_API_PRIVATE(bool_t) _cc_mutex_unlock_cs(_cc_mutex_t *mutex_cs) {
    struct _cc_mutex_cs *mutex = (struct _cc_mutex_cs *)mutex_cs;
    LeaveCriticalSection(&mutex->cs);
    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _cc_mutex_unlock_srw(_cc_mutex_t *mutex_srw) {
    struct _cc_mutex_srw *mutex = (struct _cc_mutex_srw *)mutex_srw;
    if (mutex->owner == GetCurrentThreadId()) {
        if (--mutex->count == 0) {
            mutex->owner = 0;
            pReleaseSRWLockExclusive(&mutex->srw);
        }
    } else {
        _cc_logger_error(_T("mutex not owned by this thread"));
        return false;
    }

    return true;
}

_cc_mutex_impl_t _cc_mutex_impl_active = {_cc_create_mutex_cs,   _cc_destroy_mutex_cs, _cc_mutex_lock_cs,
                                          _cc_mutex_try_lock_cs, _cc_mutex_unlock_cs,  _CC_MUTEX_CS_};

/** Create a mutex, initialized unlocked */
_CC_API_PUBLIC(_cc_mutex_t*) _cc_create_mutex(void) {
#if _CC_WINDOWS_FORCE_MUTEX_CRITICAL_SECTIONS_
#elif __WINRT__
    _cc_mutex_impl_active.Create = _cc_create_mutex_srw;
    _cc_mutex_impl_active.Destroy = _cc_destroy_mutex_srw;
    _cc_mutex_impl_active.Lock = _cc_mutex_lock_srw;
    _cc_mutex_impl_active.TryLock = _cc_mutex_try_lock_srw;
    _cc_mutex_impl_active.Unlock = _cc_mutex_unlock_srw;
    _cc_mutex_impl_active.Type = _CC_MUTEX_SRW_;
#else
    if (pReleaseSRWLockExclusive == nullptr || pAcquireSRWLockExclusive == nullptr || pTryAcquireSRWLockExclusive == nullptr) {
        /* Try faster implementation for Windows 7 and newer */
        HMODULE kernel32 = _cc_load_windows_kernel32();
        if (kernel32) {
            /* Requires Vista: */
            pInitializeSRWLock = (pfnInitializeSRWLock)GetProcAddress(kernel32, "InitializeSRWLock");
            pReleaseSRWLockExclusive = (pfnReleaseSRWLockExclusive)GetProcAddress(kernel32, "ReleaseSRWLockExclusive");
            pAcquireSRWLockExclusive = (pfnAcquireSRWLockExclusive)GetProcAddress(kernel32, "AcquireSRWLockExclusive");
            /* Requires 7: */
            pTryAcquireSRWLockExclusive =
                (pfnTryAcquireSRWLockExclusive)GetProcAddress(kernel32, "TryAcquireSRWLockExclusive");
            if (pReleaseSRWLockExclusive && pAcquireSRWLockExclusive && pTryAcquireSRWLockExclusive) {
                _cc_mutex_impl_active.Create = _cc_create_mutex_srw;
                _cc_mutex_impl_active.Destroy = _cc_destroy_mutex_srw;
                _cc_mutex_impl_active.Lock = _cc_mutex_lock_srw;
                _cc_mutex_impl_active.TryLock = _cc_mutex_try_lock_srw;
                _cc_mutex_impl_active.Unlock = _cc_mutex_unlock_srw;
                _cc_mutex_impl_active.Type = _CC_MUTEX_SRW_;
            }
        }
    }
#endif
    return _cc_mutex_impl_active.Create();
}
/* Lock the mutex */
_CC_API_PUBLIC(bool_t) _cc_mutex_lock(_cc_mutex_t *mutex) {
    if (mutex == nullptr) {
        _cc_logger_error(_T("Passed a nullptr mutex"));
        return false;
    }
    return _cc_mutex_impl_active.Lock(mutex);
}

/* try lock the mutex */
_CC_API_PUBLIC(int) _cc_mutex_try_lock(_cc_mutex_t *mutex) {
    if (mutex == nullptr) {
        _cc_logger_error(_T("Passed a nullptr mutex"));
        return false;
    }
    return _cc_mutex_impl_active.TryLock(mutex);
}

/**/
_CC_API_PUBLIC(bool_t) _cc_mutex_unlock(_cc_mutex_t *mutex) {
    if (mutex == nullptr) {
        _cc_logger_error(_T("Passed a nullptr mutex"));
        return false;
    }
    return _cc_mutex_impl_active.Unlock(mutex);
}

_CC_API_PUBLIC(void) _cc_destroy_mutex(_cc_mutex_t **mutex) {
    if (mutex == nullptr || *mutex == nullptr) {
        _cc_logger_error(_T("Passed a nullptr mutex"));
        return;
    }
    _cc_mutex_impl_active.Destroy(*mutex);
    *mutex = nullptr;
}