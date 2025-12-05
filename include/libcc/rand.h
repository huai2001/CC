#ifndef _C_CC_RAND_H_INCLUDED_
#define _C_CC_RAND_H_INCLUDED_

#include <stdlib.h>
#include "types.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _cc_prd {
    float64_t C;
    int32_t NMax;
} _cc_prd_t;

/**/
_CC_API_PUBLIC(void) _cc_srand(uint64_t);
/**/
_CC_API_PUBLIC(int32_t) _cc_rand(int32_t);
/**/
_CC_API_PUBLIC(float32_t) _cc_randf(void);
/**/
_CC_API_PUBLIC(void) _cc_random_bytes(byte_t *buf, size_t nbytes);

#define _CC_RANDOM_MAX_ 2147483647L

/**/
_CC_API_PUBLIC(void) _cc_calculate_prd(_cc_prd_t* prd, float64_t p);
/**/
_CC_API_PUBLIC(int32_t) _cc_get_probability(_cc_prd_t* prd, int T);
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_RAND_H_INCLUDED_*/
