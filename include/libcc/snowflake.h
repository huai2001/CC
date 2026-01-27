#ifndef _C_CC_SNOWFLAKE_H_INCLUDED_
#define _C_CC_SNOWFLAKE_H_INCLUDED_

#include "types.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _cc_snowflake {
    int16_t worker_id;
    int16_t seq;
    long last;
} _cc_snowflake_t;
/**/
_CC_API_PUBLIC(void) _cc_snowflake(_cc_snowflake_t *snowflake, int16_t worker_id);
/**/
_CC_API_PUBLIC(uint64_t) _cc_snowflake_id(_cc_snowflake_t *snowflake);
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_SNOWFLAKE_H_INCLUDED_*/
