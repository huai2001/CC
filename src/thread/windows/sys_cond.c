#include "sys_thread.c.h"
#include <libcc/atomic.h>

#ifdef _CC_WINDOWS_SUPPORTED_CONDITION_

typedef void(WINAPI *fptrInitializeConditionVariable)(PCONDITION_VARIABLE);
typedef void(WINAPI *fptrWakeConditionVariable)(PCONDITION_VARIABLE);
typedef void(WINAPI *fptrWakeAllConditionVariable)(PCONDITION_VARIABLE);
typedef BOOL(WINAPI *fptrSleepConditionVariableSRW)(PCONDITION_VARIABLE, PSRWLOCK, DWORD, ULONG);
typedef void(WINAPI *fptrSleepConditionVariableCS)(PCONDITION_VARIABLE, PCRITICAL_SECTION, DWORD dwMilliseco);

static fptrInitializeConditionVariable pInitializeConditionVariable = NULL;
static fptrWakeConditionVariable pWakeConditionVariable = NULL;
static fptrWakeAllConditionVariable pWakeAllConditionVariable = NULL;
static fptrSleepConditionVariableSRW pSleepConditionVariableSRW = NULL;
static fptrSleepConditionVariableCS pSleepConditionVariableCS = NULL;

#endif
/* Create a condition variable */
_CC_API_PUBLIC(_cc_condition_t*) _cc_alloc_condition(void) {
    _cc_condition_t *cond = (_cc_condition_t*)_cc_calloc(1,sizeof(_cc_condition_t));
#ifdef _CC_WINDOWS_SUPPORTED_CONDITION_
#if __WINRT__
    pInitializeConditionVariable = InitializeConditionVariable;
    pWakeConditionVariable = WakeConditionVariable;
    pWakeAllConditionVariable = WakeAllConditionVariable;
    pSleepConditionVariableSRW = SleepConditionVariableSRW;
    pSleepConditionVariableCS = SleepConditionVariableCS;
#else
    {
        HMODULE hModuleKernel32 = _cc_load_windows_kernel32();
        pInitializeConditionVariable =
            (fptrInitializeConditionVariable)GetProcAddress(hModuleKernel32, "InitializeConditionVariable");
        if (_cc_unlikely(pInitializeConditionVariable == NULL)) {
            _cc_logger_error(_T("GetProcAddress(InitializeConditionVariable) Error Code: %ld"), GetLastError());
            _cc_free(cond);
            return NULL;
        }
        pWakeConditionVariable = (fptrWakeConditionVariable)GetProcAddress(hModuleKernel32, "WakeConditionVariable");
        pWakeAllConditionVariable =
            (fptrWakeAllConditionVariable)GetProcAddress(hModuleKernel32, "WakeAllConditionVariable");
        pSleepConditionVariableSRW =
            (fptrSleepConditionVariableSRW)GetProcAddress(hModuleKernel32, "SleepConditionVariableSRW");
        pSleepConditionVariableCS =
            (fptrSleepConditionVariableCS)GetProcAddress(hModuleKernel32, "SleepConditionVariableCS");
    }
#endif
    if (pInitializeConditionVariable) {
        pInitializeConditionVariable(&cond->cond_var);
    }
#else
    cond->cond_var = CreateEvent(NULL, false, false, NULL);
    if (cond->cond_var == NULL) {
        _cc_logger_error(_T("CreateEvent() failed"));
        _cc_free(cond);
        cond = NULL;
    }
#endif
    return (cond);
}

/* Destroy a condition variable */
_CC_API_PUBLIC(void) _cc_free_condition(_cc_condition_t *cond) {
    if (_cc_likely(cond)) {
#ifdef _CC_WINDOWS_SUPPORTED_CONDITION_

#else
        CloseHandle(cond->cond_var);
#endif
        _cc_free(cond);
    }
}

/* Restart one of the threads that are waiting on the condition variable */
_CC_API_PUBLIC(bool_t) _cc_condition_signal(_cc_condition_t *cond) {
    if (_cc_unlikely(!cond)) {
        _cc_logger_error(_T("Passed a NULL condition variable"));
        return false;
    }
#ifdef _CC_WINDOWS_SUPPORTED_CONDITION_
    if (pWakeConditionVariable) {
        pWakeConditionVariable(&cond->cond_var);
    }
#else
    SetEvent(cond->cond_var);
#endif
    return true;
}

/* Restart all threads that are waiting on the condition variable */
_CC_API_PUBLIC(bool_t) _cc_condition_broadcast(_cc_condition_t *cond) {
    if (_cc_unlikely(!cond)) {
        _cc_logger_error(_T("Passed a NULL condition variable"));
        return false;
    }
#ifdef _CC_WINDOWS_SUPPORTED_CONDITION_
    if (pWakeAllConditionVariable) {
        pWakeAllConditionVariable(&cond->cond_var);
    }
#else
    SetEvent(cond->cond_var);
#endif
    return true;
}

/**/
_CC_API_PUBLIC(int) _cc_condition_wait_timeout(_cc_condition_t *cond, _cc_mutex_t *mutex, uint32_t ms) {
    DWORD timeout;
#ifndef _CC_WINDOWS_SUPPORTED_CONDITION_
    HRESULT result = NO_ERROR;
#endif
    if (_cc_unlikely(!cond)) {
        _cc_logger_error(_T("Passed a NULL condition variable"));
        return 0;
    }

    if (_cc_unlikely(!mutex)) {
        _cc_logger_error(_T("Passed a NULL mutex"));
        return 0;
    }

#ifdef _CC_WINDOWS_SUPPORTED_CONDITION_
    if (_cc_mutex_impl_active.Type == _CC_MUTEX_SRW_) {
        int result;
        struct _cc_mutex_srw *mutex_srw = (struct _cc_mutex_srw *)mutex;
        if (mutex_srw->count != 1 || mutex_srw->owner != GetCurrentThreadId()) {
            _cc_logger_error(_T("Passed mutex is not locked or locked recursively"));
            return 0;
        }

        if (ms == _CC_MUTEX_MAXWAIT_) {
            timeout = INFINITE;
        } else {
            timeout = (DWORD)ms;
        }

        /* The mutex must be updated to the released state */
        mutex_srw->count = 0;
        mutex_srw->owner = 0;

        if (pSleepConditionVariableSRW &&
            pSleepConditionVariableSRW(&cond->cond_var, &mutex_srw->srw, timeout, 0) == false) {
            if (GetLastError() == ERROR_TIMEOUT) {
                result = _CC_MUTEX_TIMEDOUT_;
            } else {
                result = 0;
                _cc_logger_error(_T("SleepConditionVariableSRW() failed"));
            }
        } else {
            result = 0;
        }

        /* The mutex is owned by us again, regardless of status of the wait */
        _cc_assert(mutex_srw->count == 0 && mutex_srw->owner == 0);
        mutex_srw->count = 1;
        mutex_srw->owner = GetCurrentThreadId();
        return result;
    } else if (pSleepConditionVariableCS) {
        struct _cc_mutex_cs *mutex_cs = (struct _cc_mutex_cs *)mutex;
        pSleepConditionVariableCS(&cond->cond_var, &mutex_cs->cs, ms);
    }
    return 0;
#else
    result = SignalObjectAndWait(&mutex->cs, cond->cond_var, ms / 1000, false);
    _cc_mutex_lock(mutex);
    if (result == WAIT_TIMEOUT) {
        return _CC_MUTEX_TIMEDOUT_;
    }
    return result;
#endif
}

/* Wait on the condition variable, unlocking the provided mutex.
 The mutex must be locked before entering this function!
 */
_CC_API_PUBLIC(bool_t) _cc_condition_wait(_cc_condition_t *cond, _cc_mutex_t *mutex) {
    if (_cc_unlikely(!cond)) {
        _cc_logger_error(_T("Passed a NULL condition variable"));
        return false;
    }
#ifdef _CC_WINDOWS_SUPPORTED_CONDITION_
    if (_cc_condition_wait_timeout(cond, mutex, _CC_MUTEX_MAXWAIT_) != 0) {
        return false;
    }
#else
    SignalObjectAndWait(&mutex->cs, cond->cond_var, INFINITE, false);
    _cc_mutex_lock(mutex);
#endif
    return true;
}
