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
#include <libcc/crypto/md2.h>
#include <libcc/string.h>

static const byte_t PI_SUBST[256] = {
    0x29, 0x2E, 0x43, 0xC9, 0xA2, 0xD8, 0x7C, 0x01, 0x3D, 0x36, 0x54, 0xA1,
    0xEC, 0xF0, 0x06, 0x13, 0x62, 0xA7, 0x05, 0xF3, 0xC0, 0xC7, 0x73, 0x8C,
    0x98, 0x93, 0x2B, 0xD9, 0xBC, 0x4C, 0x82, 0xCA, 0x1E, 0x9B, 0x57, 0x3C,
    0xFD, 0xD4, 0xE0, 0x16, 0x67, 0x42, 0x6F, 0x18, 0x8A, 0x17, 0xE5, 0x12,
    0xBE, 0x4E, 0xC4, 0xD6, 0xDA, 0x9E, 0xDE, 0x49, 0xA0, 0xFB, 0xF5, 0x8E,
    0xBB, 0x2F, 0xEE, 0x7A, 0xA9, 0x68, 0x79, 0x91, 0x15, 0xB2, 0x07, 0x3F,
    0x94, 0xC2, 0x10, 0x89, 0x0B, 0x22, 0x5F, 0x21, 0x80, 0x7F, 0x5D, 0x9A,
    0x5A, 0x90, 0x32, 0x27, 0x35, 0x3E, 0xCC, 0xE7, 0xBF, 0xF7, 0x97, 0x03,
    0xFF, 0x19, 0x30, 0xB3, 0x48, 0xA5, 0xB5, 0xD1, 0xD7, 0x5E, 0x92, 0x2A,
    0xAC, 0x56, 0xAA, 0xC6, 0x4F, 0xB8, 0x38, 0xD2, 0x96, 0xA4, 0x7D, 0xB6,
    0x76, 0xFC, 0x6B, 0xE2, 0x9C, 0x74, 0x04, 0xF1, 0x45, 0x9D, 0x70, 0x59,
    0x64, 0x71, 0x87, 0x20, 0x86, 0x5B, 0xCF, 0x65, 0xE6, 0x2D, 0xA8, 0x02,
    0x1B, 0x60, 0x25, 0xAD, 0xAE, 0xB0, 0xB9, 0xF6, 0x1C, 0x46, 0x61, 0x69,
    0x34, 0x40, 0x7E, 0x0F, 0x55, 0x47, 0xA3, 0x23, 0xDD, 0x51, 0xAF, 0x3A,
    0xC3, 0x5C, 0xF9, 0xCE, 0xBA, 0xC5, 0xEA, 0x26, 0x2C, 0x53, 0x0D, 0x6E,
    0x85, 0x28, 0x84, 0x09, 0xD3, 0xDF, 0xCD, 0xF4, 0x41, 0x81, 0x4D, 0x52,
    0x6A, 0xDC, 0x37, 0xC8, 0x6C, 0xC1, 0xAB, 0xFA, 0x24, 0xE1, 0x7B, 0x08,
    0x0C, 0xBD, 0xB1, 0x4A, 0x78, 0x88, 0x95, 0x8B, 0xE3, 0x63, 0xE8, 0x6D,
    0xE9, 0xCB, 0xD5, 0xFE, 0x3B, 0x00, 0x1D, 0x39, 0xF2, 0xEF, 0xB7, 0x0E,
    0x66, 0x58, 0xD0, 0xE4, 0xA6, 0x77, 0x72, 0xF8, 0xEB, 0x75, 0x4B, 0x0A,
    0x31, 0x44, 0x50, 0xB4, 0x8F, 0xED, 0x1F, 0x1A, 0xDB, 0x99, 0x8D, 0x33,
    0x9F, 0x11, 0x83, 0x14};

/*
 * MD2 context setup
 */
_CC_API_PUBLIC(void) _cc_md2_init(_cc_md2_t *ctx) {
    memset(ctx->cksum, 0, 16);
    memset(ctx->state, 0, 46);
    memset(ctx->buffer, 0, 16);
    ctx->left = 0;
}

#ifndef _CC_MD2_PROCESS_ALT
_CC_API_PUBLIC(void) _cc_md2_process(_cc_md2_t *ctx) {
    int i, j;
    byte_t t = 0;

    for (i = 0; i < 16; i++) {
        ctx->state[i + 16] = ctx->buffer[i];
        ctx->state[i + 32] = (byte_t)(ctx->buffer[i] ^ ctx->state[i]);
    }

    for (i = 0; i < 18; i++) {
        for (j = 0; j < 48; j++) {
            ctx->state[j] = (byte_t)(ctx->state[j] ^ PI_SUBST[t]);
            t = ctx->state[j];
        }
        t = (byte_t)(t + i);
    }

    t = ctx->cksum[15];

    for (i = 0; i < 16; i++) {
        ctx->cksum[i] = (byte_t)(ctx->cksum[i] ^ PI_SUBST[ctx->buffer[i] ^ t]);
        t = ctx->cksum[i];
    }
}
#endif /* !CC_MD2_PROCESS_ALT */

/*
 * MD2 process buffer
 */
_CC_API_PUBLIC(void) _cc_md2_update(_cc_md2_t *ctx, const byte_t *input, size_t ilen) {
    size_t fill;

    while (ilen > 0) {
        if (ilen > 16 - ctx->left) {
            fill = 16 - ctx->left;
        } else {
            fill = ilen;
        }

        memcpy(ctx->buffer + ctx->left, input, fill);

        ctx->left += fill;
        input += fill;
        ilen -= fill;

        if (ctx->left == 16) {
            ctx->left = 0;
            _cc_md2_process(ctx);
        }
    }
}

/*
 * MD2 final digest
 */
_CC_API_PUBLIC(void) _cc_md2_final(_cc_md2_t *ctx, byte_t *output) {
    size_t i;
    byte_t x;

    x = (byte_t)(16 - ctx->left);

    for (i = ctx->left; i < 16; i++)
        ctx->buffer[i] = x;

    _cc_md2_process(ctx);

    memcpy(ctx->buffer, ctx->cksum, 16);

    _cc_md2_process(ctx);

    memcpy(output, ctx->state, 16);
}

/*
    Digests a file.
 */
_CC_API_PUBLIC(bool_t) _cc_md2_fp(FILE *fp, tchar_t *output) {
    byte_t md[_CC_MD2_DIGEST_LENGTH_];
    byte_t buf[1024 * 16];
    size_t i;
    long seek_cur = 0;
    _cc_md2_t c;

    if (fp == nullptr) {
        return false;
    }

    _cc_md2_init(&c);

    seek_cur = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    while ((i = fread(buf, sizeof(byte_t), _cc_countof(buf), fp))) {
        _cc_md2_update(&c, buf, i);
    }

    _cc_md2_final(&c, &(md[0]));

    fseek(fp, seek_cur, SEEK_CUR);

    _cc_bytes2hex(md, _CC_MD2_DIGEST_LENGTH_, output, _CC_MD2_DIGEST_LENGTH_ * 2);

    return true;
}

/*
    Digests a file.
 */
_CC_API_PUBLIC(bool_t) _cc_md2file(const tchar_t *filename, tchar_t *output) {
    FILE *fp = _tfopen(filename, _T("rb"));

    if (fp) {
        _cc_md2_fp(fp, output);
        fclose(fp);
        return true;
    }
    return false;
}

/*
 * output = MD2( input buffer )
 */
_CC_API_PUBLIC(void) _cc_md2(const byte_t *input, size_t length, tchar_t *output) {
    _cc_md2_t c;
    byte_t md[_CC_MD2_DIGEST_LENGTH_];

    _cc_md2_init(&c);
    _cc_md2_update(&c, input, length);
    _cc_md2_final(&c, &(md[0]));

    _cc_bytes2hex(md, _CC_MD2_DIGEST_LENGTH_, output, _CC_MD2_DIGEST_LENGTH_ * 2);
}
