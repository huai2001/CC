/*
 * Copyright .Qiu<huai2011@163.com>. and other libCC contributors.
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
#include <cc/hmac.h>
#include <cc/string.h>
#include <cc/alloc.h>

#define MAX_HASHLEN 64
#define MAX_BLOCKLEN 128

typedef void (*_hmac_init)(pvoid_t);
typedef void (*_hmac_update)(pvoid_t, const byte_t*, size_t);
typedef void (*_hmac_final)(pvoid_t, byte_t*);

struct _cc_hmac {
    int block_lenght;
    int hash_lenght;
    int key_lenght;
    pvoid_t hash;
    _hmac_init init;
    _hmac_update update;
    _hmac_final  final;
    uint8_t key[MAX_BLOCKLEN];
};

static void __sha224_init(_cc_sha256_t *ctx) {
    _cc_sha256_init(ctx, true);
}

static void __sha256_init(_cc_sha256_t *ctx) {
    _cc_sha256_init(ctx, false);
}

static void __sha384_init(_cc_sha512_t *ctx) {
    _cc_sha512_init(ctx, true);
}

static void __sha512_init(_cc_sha512_t *ctx) {
    _cc_sha512_init(ctx, false);
}

static void _hmac_init_block(_cc_hmac_t *c, byte_t *block, byte_t cp) {
    int i;
    for (i = 0; i < c->key_lenght; i++) {
        block[i] = *(c->key + i) ^ cp;
    }
    for (i = c->key_lenght; i < c->block_lenght; i++) {
        block[i] = cp;
    }
}

_cc_hmac_t* _cc_hmac_alloc(byte_t type) {
    _cc_hmac_t *hmac = (_cc_hmac_t*)_cc_malloc(sizeof(_cc_hmac_t));
    hmac->key_lenght = 0;
    switch(type) {
    case _CC_HMAC_MD5_:
        hmac->hash_lenght  = _CC_MD5_DIGEST_LENGTH_;
        hmac->block_lenght = 64;
        hmac->hash = _cc_malloc(sizeof(_cc_md5_t));
        hmac->init = (_hmac_init)_cc_md5_init;
        hmac->update = (_hmac_update)_cc_md5_update;
        hmac->final = (_hmac_final)_cc_md5_final;
        break;
    case _CC_HMAC_SHA1_:
        hmac->hash_lenght  = _CC_SHA1_DIGEST_LENGTH_;
        hmac->block_lenght = 64;
        hmac->hash = _cc_malloc(sizeof(_cc_sha1_t));
        hmac->init = (_hmac_init)_cc_sha1_init;
        hmac->update = (_hmac_update)_cc_sha1_update;
        hmac->final = (_hmac_final)_cc_sha1_final;
        break;
    case _CC_HMAC_SHA224_:
        hmac->hash_lenght  = _CC_SHA224_DIGEST_LENGTH_;
        hmac->block_lenght = 64;
        hmac->hash = _cc_malloc(sizeof(_cc_sha256_t));
        hmac->init = (_hmac_init)__sha224_init;
        hmac->update = (_hmac_update)_cc_sha256_update;
        hmac->final = (_hmac_final)_cc_sha256_final;
        break;
    case _CC_HMAC_SHA256_:
        hmac->hash_lenght  = _CC_SHA256_DIGEST_LENGTH_;
        hmac->block_lenght = 64;
        hmac->hash = _cc_malloc(sizeof(_cc_sha256_t));
        hmac->init = (_hmac_init)__sha256_init;
        hmac->update = (_hmac_update)_cc_sha256_update;
        hmac->final = (_hmac_final)_cc_sha256_final;
        break;
    case _CC_HMAC_SHA384_:
        hmac->hash_lenght  = _CC_SHA384_DIGEST_LENGTH_;
        hmac->block_lenght = 128;
        hmac->hash = _cc_malloc(sizeof(_cc_sha512_t));
        hmac->init = (_hmac_init)__sha384_init;
        hmac->update = (_hmac_update)_cc_sha512_update;
        hmac->final = (_hmac_final)_cc_sha512_final;
        break;
    case _CC_HMAC_SHA512_:
        hmac->hash_lenght  = _CC_SHA512_DIGEST_LENGTH_;
        hmac->block_lenght = 128;
        hmac->hash = _cc_malloc(sizeof(_cc_sha512_t));
        hmac->init = (_hmac_init)__sha512_init;
        hmac->update = (_hmac_update)_cc_sha512_update;
        hmac->final = (_hmac_final)_cc_sha512_final;
        break;
    default:
        _cc_free(hmac);
        return NULL;
    }

    if (hmac->hash == NULL) {
        _cc_free(hmac);
        return NULL;
    }

    return hmac;
}

void _cc_hmac_free(_cc_hmac_t *ctx) {
    if (ctx) {
        return;
    }

    _cc_free(ctx->hash);
    _cc_free(ctx);
}


void _cc_hmac_init(_cc_hmac_t *c, const byte_t *key, size_t length) {

    byte_t block[MAX_BLOCKLEN];

    if (length > (size_t)c->block_lenght) {
        c->init(c->hash);
        c->update(c->hash, key, length);
        c->final(c->hash, c->key);
        c->key_lenght = c->hash_lenght;
    } else {
        c->key_lenght = (int)length;
        memcpy(c->key, key, length);
    }

    _hmac_init_block(c, block, 0x36);

    c->init(c->hash);
    c->update(c->hash, block, c->block_lenght);
}

void _cc_hmac_update(_cc_hmac_t *c, const byte_t *input, size_t length) {
    c->update(c->hash, input, length);
}

int _cc_hmac_final(_cc_hmac_t *c, byte_t *output, int length) {
    byte_t block[MAX_BLOCKLEN];

    if (length < c->hash_lenght) {
        return 0;
    }

    c->final(c->hash, output);
    c->init(c->hash);
    _hmac_init_block(c, block, 0x5C);
    c->update(c->hash, block, c->block_lenght);
    c->update(c->hash, output, c->hash_lenght);
    c->final(c->hash, output);

    return c->hash_lenght;
}

/*
 * 
 */
int _cc_hmac(byte_t type, const byte_t *input, size_t ilen, const byte_t *key, size_t key_length, tchar_t *output) {
    int r;
    byte_t digest[MAX_BLOCKLEN];
    _cc_hmac_t *hmac = _cc_hmac_alloc(type);
    if (hmac == NULL) {
        return 0;
    }
    _cc_hmac_init(hmac, key, key_length);
    _cc_hmac_update(hmac, input, ilen);
    r = _cc_hmac_final(hmac, digest, hmac->hash_lenght);
    _cc_bytes2hex(digest, hmac->hash_lenght, output, hmac->hash_lenght * 2);

    _cc_hmac_free(hmac);

    return r;
}
