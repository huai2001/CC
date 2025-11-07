
#ifndef _C_CC_TIMEOUT_H_INCLUDED_
#define _C_CC_TIMEOUT_H_INCLUDED_

#include "event.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**/
_CC_API_PUBLIC(bool_t) _cc_register_timeout(_cc_async_event_t*);
/**/
_CC_API_PUBLIC(_cc_event_t*) _cc_add_event_timeout(_cc_async_event_t* async, uint32_t milliseconds, _cc_event_callback_t callback, uintptr_t data);
/**/
_CC_API_PUBLIC(bool_t) _cc_kill_event_timeout(_cc_async_event_t *async, _cc_event_t *e);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _C_CC_TIMEOUT_H_INCLUDED_ */
