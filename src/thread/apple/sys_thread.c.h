#ifndef _C_CC_SYS_APPLE_GCD_C_H_INCLUDED_
#define _C_CC_SYS_APPLE_GCD_C_H_INCLUDED_

#include <libcc/alloc.h>
#include <libcc/thread.h>
#include <libcc/time.h>
#include <errno.h>
#include <semaphore.h>

#include <dispatch/dispatch.h>
#include <os/lock.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

struct _cc_semaphore {
    dispatch_semaphore_t sem;
};

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_SYS_APPLE_GCD_C_H_INCLUDED_*/
