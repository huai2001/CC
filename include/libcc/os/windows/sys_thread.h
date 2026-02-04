#ifndef _C_CC_WINDOWS_THREAD_H_INCLUDED_
#define _C_CC_WINDOWS_THREAD_H_INCLUDED_

#include "../windows.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define _CC_ONCE_INIT_ { 0, { NULL } }

typedef struct _cc_once {
    unsigned char unused;
    INIT_ONCE init_once;
} _cc_once_t;

/* This is the system-independent thread info structure */
typedef HANDLE _cc_thread_handle_t;

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_WINDOWS_THREAD_H_INCLUDED_*/




