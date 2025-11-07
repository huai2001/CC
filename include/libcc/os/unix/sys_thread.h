#ifndef _C_CC_UNIX_THREAD_HEAD_FILE_
#define _C_CC_UNIX_THREAD_HEAD_FILE_

#include <pthread.h>
#include "../unix.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define _CC_ONCE_INIT_ PTHREAD_ONCE_INIT

typedef pthread_once_t _cc_once_t;
/* This is the system-independent thread info structure */
typedef pthread_t _cc_thread_handle_t;

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_UNIX_THREAD_HEAD_FILE_*/




