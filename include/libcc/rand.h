#ifndef _C_CC_RAND_H_INCLUDED_
#define _C_CC_RAND_H_INCLUDED_

#include <stdlib.h>
#include "types.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define _CC_RANDOM_MAX_ 2147483647L

/*probability distribution*/
typedef struct _cc_prd {
    float64_t c;
    int32_t nmax;
} _cc_prd_t;

/**/
_CC_API_PUBLIC(void) _cc_srand(uint64_t);
/**/
_CC_API_PUBLIC(int32_t) _cc_rand(int32_t);
/**/
_CC_API_PUBLIC(float32_t) _cc_randf(void);
/**/
_CC_API_PUBLIC(void) _cc_random_bytes(byte_t *buf, size_t nbytes);

/*probability distribution*/
_CC_API_PUBLIC(void) _cc_prd(_cc_prd_t* prd, float64_t p);
/*probability distribution*/
_CC_API_PUBLIC(int32_t) _cc_get_probability(_cc_prd_t* prd, int T);
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_RAND_H_INCLUDED_*/
