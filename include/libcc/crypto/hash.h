#ifndef _C_CC_CRYPTO_HASH_H_INCLUDED_
#define _C_CC_CRYPTO_HASH_H_INCLUDED_

#include "../os.h"

#define _CC_SHA1_              1
#define _CC_SHA224_            2
#define _CC_SHA256_            3
#define _CC_SHA384_            4
#define _CC_SHA512_            5
#define _CC_MD5_               6

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _cc_hash {
    uintptr_t handle;
    byte_t method;
    void (*init)(struct _cc_hash *);
    void (*update)(struct _cc_hash *, const byte_t *, size_t);
    void (*final)(struct _cc_hash *, byte_t *, int32_t *);
    void (*free)(struct _cc_hash *);
} _cc_hash_t;

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _C_CC_CRYPTO_HASH_H_INCLUDED_ */