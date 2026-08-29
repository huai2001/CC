#ifndef _C_CC_THREAD_H_INCLUDED_
#define _C_CC_THREAD_H_INCLUDED_

#include "atomic.h"
#include "logger.h"
#include "sds.h"

#ifdef __CC_WINDOWS__
#include "os/windows/sys_thread.h"
#else
#include "os/unix/sys_thread.h"
#endif

#include "list.h"
#include "mutex.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#if __CC_STDC_VERSION__ >= 98
    #define _cc_thread_local_t _Thread_local
#elif defined(__CC_GNUC__)
    #define _cc_thread_local_t __thread
#elif defined(__CC_MSVC__)
    #define _cc_thread_local_t __declspec(thread)
#elif defined (__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)
    #define _cc_thread_local_t _Thread_local
#else
    #define _cc_thread_local_t "Unsupported thread-local storage"
#endif

/**/
typedef struct _cc_thread _cc_thread_t;

typedef void (*_cc_once_callback_t)(void);
/**/
typedef int32_t (*_cc_thread_callback_t)(pvoid_t);

typedef enum {
    _CC_THREAD_STATE_ALIVE_,
    _CC_THREAD_STATE_DETACHED_,
    _CC_THREAD_STATE_COMPLETE_,
} _CC_THREAD_STATE_;

struct _cc_thread {
    int32_t status;
    _cc_atomic32_t state;
    size_t thread_id;
    /* 0 for default, >0 for user-specified stack size. */
    size_t stack_size;
    _cc_thread_handle_t handle;
    _cc_sds_t name;
    _cc_thread_callback_t callback;
    pvoid_t args;
};

/*
 * The thread priority
 * Note: On many systems you require special privileges to set high priority.
 */
typedef enum {
    _CC_THREAD_PRIORITY_LOW_,
    _CC_THREAD_PRIORITY_NORMAL_,
    _CC_THREAD_PRIORITY_HIGH_
} _CC_THREAD_PRIORITY_EMUM_;

/* This is the function called to run a thread */
_CC_API_DYLIB_PRIVATE(void) _cc_thread_running_function(pvoid_t);

/**/
_CC_API_PUBLIC(void)  _cc_once(_cc_once_t* guard, _cc_once_callback_t callback);
/**/
_CC_API_PUBLIC(_cc_thread_t *) _cc_thread(_cc_thread_callback_t callback, const tchar_t *name, pvoid_t args);
/**/
_CC_API_PUBLIC(_cc_thread_t *) _cc_thread_with_stacksize(_cc_thread_callback_t callback, const tchar_t *name, size_t stack_size, pvoid_t args);
/**/
_CC_API_PUBLIC(bool_t) _cc_thread_start(_cc_thread_callback_t callback, const tchar_t *name, pvoid_t args);
/**/
_CC_API_PUBLIC(bool_t) _cc_thread_priority(_CC_THREAD_PRIORITY_EMUM_);
/**/
_CC_API_PUBLIC(void) _cc_wait_thread(_cc_thread_t *, int32_t *);
/**/
_CC_API_PUBLIC(void) _cc_detach_thread(_cc_thread_t *);
/**/
_CC_API_PUBLIC(size_t) _cc_get_thread_id(_cc_thread_t *);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_THREAD_H_INCLUDED_*/
