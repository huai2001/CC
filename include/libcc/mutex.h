#ifndef _C_CC_MUTEX_H_INCLUDED_
#define _C_CC_MUTEX_H_INCLUDED_

#include "types.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif
/**
 * Synchronization functions which can time out return this value
 *  if they time out.
 */
#define _CC_MUTEX_TIMEDOUT_ 1

/**
 * This is the timeout value which corresponds to never time out
 */
#define _CC_MUTEX_MAXWAIT_ (~(int32_t)0)

/**/
struct _cc_mutex;
typedef struct _cc_mutex _cc_mutex_t;

/**/
_CC_API_PUBLIC(_cc_mutex_t*) _cc_alloc_mutex(void);
/**/
_CC_API_PUBLIC(bool_t) _cc_mutex_lock(_cc_mutex_t*);
/**/
_CC_API_PUBLIC(int) _cc_mutex_try_lock(_cc_mutex_t*);
/**/
_CC_API_PUBLIC(bool_t) _cc_mutex_unlock(_cc_mutex_t*);
/**/
_CC_API_PUBLIC(void) _cc_free_mutex(_cc_mutex_t*);

/**/
typedef struct _cc_semaphore _cc_semaphore_t;

/**/
_CC_API_PUBLIC(_cc_semaphore_t*) _cc_alloc_semaphore(int32_t);
/**/
_CC_API_PUBLIC(void) _cc_free_semaphore(_cc_semaphore_t*);

/**/
_CC_API_PUBLIC(int) _cc_semaphore_wait(_cc_semaphore_t*);
/**/
_CC_API_PUBLIC(int) _cc_semaphore_try_wait(_cc_semaphore_t*);

/**/
_CC_API_PUBLIC(int) _cc_semaphore_wait_timeout(_cc_semaphore_t*, uint32_t);
/**/
_CC_API_PUBLIC(bool_t) _cc_semaphore_post(_cc_semaphore_t*);
/**/
_CC_API_PUBLIC(uint32_t) _cc_semaphore_value(_cc_semaphore_t*);

/**/
typedef struct _cc_condition _cc_condition_t;

/**/
_CC_API_PUBLIC(_cc_condition_t*) _cc_alloc_condition(void);
/**/
_CC_API_PUBLIC(void) _cc_free_condition(_cc_condition_t*);
/**/
_CC_API_PUBLIC(bool_t) _cc_condition_signal(_cc_condition_t*);
/**/
_CC_API_PUBLIC(bool_t) _cc_condition_broadcast(_cc_condition_t*);
/**/
_CC_API_PUBLIC(bool_t) _cc_condition_wait(_cc_condition_t*, _cc_mutex_t*);
/**/
_CC_API_PUBLIC(int) _cc_condition_wait_timeout(_cc_condition_t*, _cc_mutex_t*, uint32_t);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_MUTEX_H_INCLUDED_*/
