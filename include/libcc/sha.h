/*
 * Copyright libcc.cn@gmail.com. and other libcc contributors.
 * All rights reserved.org>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:

 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
*/
#ifndef _C_CC_SHA_H_INCLUDED_
#define _C_CC_SHA_H_INCLUDED_

#include "core.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define _CC_SHA1_DIGEST_LENGTH_       20
#define _CC_SHA224_DIGEST_LENGTH_     28
#define _CC_SHA256_DIGEST_LENGTH_     32
#define _CC_SHA384_DIGEST_LENGTH_     48
#define _CC_SHA512_DIGEST_LENGTH_     64

#define _CC_KECCAK1600_WIDTH_         1600


/**
 * @brief          SHA-1 context structure
 */
typedef struct _cc_sha1 {
    uint32_t total[2]; /*!< number of bytes processed  */
    uint32_t state[5]; /*!< intermediate digest state  */
    byte_t buffer[64]; /*!< data block being processed */
} _cc_sha1_t;

/**
 * @brief          SHA-256 context structure
 */
typedef struct _cc_sha256 {
    uint32_t total[2]; /*!< number of bytes processed  */
    uint32_t state[8]; /*!< intermediate digest state  */
    byte_t buffer[64]; /*!< data block being processed */
    bool_t is224;      /*!< 0 => SHA-256, else SHA-224 */
} _cc_sha256_t;

/**
 * @brief          SHA-256 context structure
 */
typedef struct _cc_sha512 {
    uint64_t total[2];  /*!< number of bytes processed  */
    uint64_t state[8];  /*!< intermediate digest state  */
    byte_t buffer[128]; /*!< data block being processed */
    bool_t is384;       /*!< 0 => SHA-512, else SHA-384 */
} _cc_sha512_t;

typedef size_t (sha3_absorb_fn)(void *vctx, const void *inp, size_t len);
typedef int (sha3_final_fn)(byte_t *md, void *vctx);

typedef struct prov_sha3_meth {
    sha3_absorb_fn *absorb;
    sha3_final_fn *final;
} prov_sha3_meth_t;

/**
 * @brief          SHA-3 context structure
 */
typedef struct _cc_sha3 {
    uint64_t A[5][5];
    size_t block_size;          /* cached ctx->digest->block_size */
    size_t md_size;             /* output length, variable in XOF */
    size_t bufsz;               /* used bytes in below buffer */
    byte_t buf[_CC_KECCAK1600_WIDTH_ / 8 - 32];
    byte_t pad;
    prov_sha3_meth_t meth;
} _cc_sha3_t;

/**
 * @brief          Initialize SHA-1 context
 *
 * @param ctx      SHA-1 context to be initialized
 */
_CC_API_PUBLIC(void) _cc_sha1_init(_cc_sha1_t* ctx);
/**
 * @brief          SHA-1 process buffer
 *
 * @param ctx      SHA-1 context
 * @param input    buffer holding the  data
 * @param ilen     length of the input data
 */
_CC_API_PUBLIC(void)
_cc_sha1_update(_cc_sha1_t* ctx, const byte_t* input, size_t ilen);

/**
 * @brief          SHA-1 final digest
 *
 * @param ctx      SHA-1 context
 * @param output   SHA-1 checksum result
 */
_CC_API_PUBLIC(void) _cc_sha1_final(_cc_sha1_t* ctx, byte_t* output);

/**
 * @brief          Output = SHA-1( input buffer )
 *
 * @param input    buffer holding the  data
 * @param ilen     length of the input data
 * @param output   SHA-1
 */
_CC_API_PUBLIC(void) _cc_sha1(const byte_t* input, size_t ilen, tchar_t* output);
/**
 * @brief         Digests a file.
 *
 * @param fp       FILE handle
 * @param output   SHA1 checksum result
 */
_CC_API_PUBLIC(bool_t) _cc_sha1_fp(FILE* fp, tchar_t* output);
/*
    Digests a file.
 */
_CC_API_PUBLIC(bool_t) _cc_sha1file(const tchar_t* filename, tchar_t* output);

/**
 * @brief          Initialize SHA-256 context
 *
 * @param ctx      SHA-256 context to be initialized
 * @param is224    0 = use SHA256, 1 = use SHA224
 */
_CC_API_PUBLIC(void) _cc_sha256_init(_cc_sha256_t* ctx, bool_t is224);
/**
 * @brief          SHA-256 process buffer
 *
 * @param ctx      SHA-256 context
 * @param input    buffer holding the  data
 * @param ilen     length of the input data
 */
_CC_API_PUBLIC(void)
_cc_sha256_update(_cc_sha256_t* ctx, const byte_t* input, size_t ilen);

/**
 * @brief          SHA-256 final digest
 *
 * @param ctx      SHA-256 context
 * @param output   SHA-256 checksum result
 */
_CC_API_PUBLIC(void) _cc_sha256_final(_cc_sha256_t* ctx, byte_t* output);

/**
 * @brief          Output = SHA-256( input buffer )
 *
 * @param input    buffer holding the  data
 * @param ilen     length of the input data
 * @param output   SHA-256
 * @param is224    0 = use SHA256, 1 = use SHA224
 */
_CC_API_PUBLIC(void)
_cc_sha256(const byte_t* input, size_t ilen, tchar_t* output, bool_t is224);
/**
 * @brief         Digests a file.
 *
 * @param fp       FILE handle
 * @param output   SHA256 checksum result
 */
_CC_API_PUBLIC(bool_t) _cc_sha256_fp(FILE *fp, tchar_t *output, bool_t is224);
/**
 * @brief          Output = SHA-256( input file path )
 *
 * @param filename    buffer holding the  data
 * @param output   SHA-256
 * @param is384    0 = use SHA256, 1 = use SHA224
 */
_CC_API_PUBLIC(bool_t)
_cc_sha256file(const tchar_t* filename, tchar_t* output, bool_t is224);

/**
 * @brief          Initialize SHA-512 context
 *
 * @param ctx      SHA-512 context to be initialized
 * @param is384    0 = use SHA512, 1 = use SHA384
 */
_CC_API_PUBLIC(void) _cc_sha512_init(_cc_sha512_t* ctx, bool_t is384);
/**
 * @brief          SHA-512 process buffer
 *
 * @param ctx      SHA-512 context
 * @param input    buffer holding the  data
 * @param ilen     length of the input data
 */
_CC_API_PUBLIC(void)
_cc_sha512_update(_cc_sha512_t* ctx, const byte_t* input, size_t ilen);

/**
 * @brief          SHA-512 final digest
 *
 * @param ctx      SHA-512 context
 * @param output   SHA-512 checksum result
 */
_CC_API_PUBLIC(void) _cc_sha512_final(_cc_sha512_t* ctx, byte_t* output);

/**
 * @brief       Output = SHA-512( input buffer )
 *
 * @param input      buffer holding the  data
 * @param ilen        length of the input data
 * @param output    SHA-512
 * @param is384    0 = use SHA512, 1 = use SHA384
 */
_CC_API_PUBLIC(void)
_cc_sha512(const byte_t* input, size_t ilen, tchar_t* output, bool_t is384);
/**
 * @brief         Digests a file.
 *
 * @param fp       FILE handle
 * @param output   SHA-512 checksum result
 */
_CC_API_PUBLIC(bool_t) _cc_sha512_fp(FILE *fp, tchar_t *output, bool_t is384);
/**
 * @brief       Output = SHA-512(  input file path )

 * @param filename    buffer holding the  data
 * @param output    SHA-512
 * @param is384    0 = use SHA512, 1 = use SHA384
 */
_CC_API_PUBLIC(bool_t)
_cc_sha512file(const tchar_t* filename, tchar_t* output, bool_t is384);

/**
 * @brief          Initialize SHA-3 context
 *
 * @param ctx      SHA-3 context to be initialized
 * @param pad      pad
 * @param bitlen   bitlen
 */
_CC_API_PUBLIC(void) _cc_sha3_init(_cc_sha3_t* ctx, byte_t pad, size_t bitlen);
/**
 * @brief          SHA-3 process buffer
 *
 * @param ctx      SHA-3 context
 * @param input    buffer holding the  data
 * @param ilen     length of the input data
 */
_CC_API_PUBLIC(void)
_cc_sha3_update(_cc_sha3_t* ctx, const byte_t* input, size_t ilen);

/**
 * @brief          SHA-3 final digest
 *
 * @param ctx      SHA-3 context
 * @param output   SHA-3 checksum result
 */
_CC_API_PUBLIC(void) _cc_sha3_final(_cc_sha3_t* ctx, byte_t* output);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_SHA_H_INCLUDED_*/
