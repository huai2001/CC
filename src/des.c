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
#include <cc/des.h>
/*
 * 32-bit integer manipulation macros (big endian)
 */
#ifndef GET_UINT32_BE
#define GET_UINT32_BE(n, b, i)                                              \
    {                                                                       \
        (n) = ((uint32_t)(b)[(i)] << 24) | ((uint32_t)(b)[(i) + 1] << 16) | \
              ((uint32_t)(b)[(i) + 2] << 8) | ((uint32_t)(b)[(i) + 3]);     \
    }
#endif

#ifndef PUT_UINT32_BE
#define PUT_UINT32_BE(n, b, i)              \
    {                                       \
        (b)[(i)] = (byte_t)((n) >> 24);     \
        (b)[(i) + 1] = (byte_t)((n) >> 16); \
        (b)[(i) + 2] = (byte_t)((n) >> 8);  \
        (b)[(i) + 3] = (byte_t)((n));       \
    }
#endif

/*
 * Expanded DES S-boxes
 */
static const uint32_t SB1[64] = {
    0x01010400, 0x00000000, 0x00010000, 0x01010404, 0x01010004, 0x00010404,
    0x00000004, 0x00010000, 0x00000400, 0x01010400, 0x01010404, 0x00000400,
    0x01000404, 0x01010004, 0x01000000, 0x00000004, 0x00000404, 0x01000400,
    0x01000400, 0x00010400, 0x00010400, 0x01010000, 0x01010000, 0x01000404,
    0x00010004, 0x01000004, 0x01000004, 0x00010004, 0x00000000, 0x00000404,
    0x00010404, 0x01000000, 0x00010000, 0x01010404, 0x00000004, 0x01010000,
    0x01010400, 0x01000000, 0x01000000, 0x00000400, 0x01010004, 0x00010000,
    0x00010400, 0x01000004, 0x00000400, 0x00000004, 0x01000404, 0x00010404,
    0x01010404, 0x00010004, 0x01010000, 0x01000404, 0x01000004, 0x00000404,
    0x00010404, 0x01010400, 0x00000404, 0x01000400, 0x01000400, 0x00000000,
    0x00010004, 0x00010400, 0x00000000, 0x01010004};

static const uint32_t SB2[64] = {
    0x80108020, 0x80008000, 0x00008000, 0x00108020, 0x00100000, 0x00000020,
    0x80100020, 0x80008020, 0x80000020, 0x80108020, 0x80108000, 0x80000000,
    0x80008000, 0x00100000, 0x00000020, 0x80100020, 0x00108000, 0x00100020,
    0x80008020, 0x00000000, 0x80000000, 0x00008000, 0x00108020, 0x80100000,
    0x00100020, 0x80000020, 0x00000000, 0x00108000, 0x00008020, 0x80108000,
    0x80100000, 0x00008020, 0x00000000, 0x00108020, 0x80100020, 0x00100000,
    0x80008020, 0x80100000, 0x80108000, 0x00008000, 0x80100000, 0x80008000,
    0x00000020, 0x80108020, 0x00108020, 0x00000020, 0x00008000, 0x80000000,
    0x00008020, 0x80108000, 0x00100000, 0x80000020, 0x00100020, 0x80008020,
    0x80000020, 0x00100020, 0x00108000, 0x00000000, 0x80008000, 0x00008020,
    0x80000000, 0x80100020, 0x80108020, 0x00108000};

static const uint32_t SB3[64] = {
    0x00000208, 0x08020200, 0x00000000, 0x08020008, 0x08000200, 0x00000000,
    0x00020208, 0x08000200, 0x00020008, 0x08000008, 0x08000008, 0x00020000,
    0x08020208, 0x00020008, 0x08020000, 0x00000208, 0x08000000, 0x00000008,
    0x08020200, 0x00000200, 0x00020200, 0x08020000, 0x08020008, 0x00020208,
    0x08000208, 0x00020200, 0x00020000, 0x08000208, 0x00000008, 0x08020208,
    0x00000200, 0x08000000, 0x08020200, 0x08000000, 0x00020008, 0x00000208,
    0x00020000, 0x08020200, 0x08000200, 0x00000000, 0x00000200, 0x00020008,
    0x08020208, 0x08000200, 0x08000008, 0x00000200, 0x00000000, 0x08020008,
    0x08000208, 0x00020000, 0x08000000, 0x08020208, 0x00000008, 0x00020208,
    0x00020200, 0x08000008, 0x08020000, 0x08000208, 0x00000208, 0x08020000,
    0x00020208, 0x00000008, 0x08020008, 0x00020200};

static const uint32_t SB4[64] = {
    0x00802001, 0x00002081, 0x00002081, 0x00000080, 0x00802080, 0x00800081,
    0x00800001, 0x00002001, 0x00000000, 0x00802000, 0x00802000, 0x00802081,
    0x00000081, 0x00000000, 0x00800080, 0x00800001, 0x00000001, 0x00002000,
    0x00800000, 0x00802001, 0x00000080, 0x00800000, 0x00002001, 0x00002080,
    0x00800081, 0x00000001, 0x00002080, 0x00800080, 0x00002000, 0x00802080,
    0x00802081, 0x00000081, 0x00800080, 0x00800001, 0x00802000, 0x00802081,
    0x00000081, 0x00000000, 0x00000000, 0x00802000, 0x00002080, 0x00800080,
    0x00800081, 0x00000001, 0x00802001, 0x00002081, 0x00002081, 0x00000080,
    0x00802081, 0x00000081, 0x00000001, 0x00002000, 0x00800001, 0x00002001,
    0x00802080, 0x00800081, 0x00002001, 0x00002080, 0x00800000, 0x00802001,
    0x00000080, 0x00800000, 0x00002000, 0x00802080};

static const uint32_t SB5[64] = {
    0x00000100, 0x02080100, 0x02080000, 0x42000100, 0x00080000, 0x00000100,
    0x40000000, 0x02080000, 0x40080100, 0x00080000, 0x02000100, 0x40080100,
    0x42000100, 0x42080000, 0x00080100, 0x40000000, 0x02000000, 0x40080000,
    0x40080000, 0x00000000, 0x40000100, 0x42080100, 0x42080100, 0x02000100,
    0x42080000, 0x40000100, 0x00000000, 0x42000000, 0x02080100, 0x02000000,
    0x42000000, 0x00080100, 0x00080000, 0x42000100, 0x00000100, 0x02000000,
    0x40000000, 0x02080000, 0x42000100, 0x40080100, 0x02000100, 0x40000000,
    0x42080000, 0x02080100, 0x40080100, 0x00000100, 0x02000000, 0x42080000,
    0x42080100, 0x00080100, 0x42000000, 0x42080100, 0x02080000, 0x00000000,
    0x40080000, 0x42000000, 0x00080100, 0x02000100, 0x40000100, 0x00080000,
    0x00000000, 0x40080000, 0x02080100, 0x40000100};

static const uint32_t SB6[64] = {
    0x20000010, 0x20400000, 0x00004000, 0x20404010, 0x20400000, 0x00000010,
    0x20404010, 0x00400000, 0x20004000, 0x00404010, 0x00400000, 0x20000010,
    0x00400010, 0x20004000, 0x20000000, 0x00004010, 0x00000000, 0x00400010,
    0x20004010, 0x00004000, 0x00404000, 0x20004010, 0x00000010, 0x20400010,
    0x20400010, 0x00000000, 0x00404010, 0x20404000, 0x00004010, 0x00404000,
    0x20404000, 0x20000000, 0x20004000, 0x00000010, 0x20400010, 0x00404000,
    0x20404010, 0x00400000, 0x00004010, 0x20000010, 0x00400000, 0x20004000,
    0x20000000, 0x00004010, 0x20000010, 0x20404010, 0x00404000, 0x20400000,
    0x00404010, 0x20404000, 0x00000000, 0x20400010, 0x00000010, 0x00004000,
    0x20400000, 0x00404010, 0x00004000, 0x00400010, 0x20004010, 0x00000000,
    0x20404000, 0x20000000, 0x00400010, 0x20004010};

static const uint32_t SB7[64] = {
    0x00200000, 0x04200002, 0x04000802, 0x00000000, 0x00000800, 0x04000802,
    0x00200802, 0x04200800, 0x04200802, 0x00200000, 0x00000000, 0x04000002,
    0x00000002, 0x04000000, 0x04200002, 0x00000802, 0x04000800, 0x00200802,
    0x00200002, 0x04000800, 0x04000002, 0x04200000, 0x04200800, 0x00200002,
    0x04200000, 0x00000800, 0x00000802, 0x04200802, 0x00200800, 0x00000002,
    0x04000000, 0x00200800, 0x04000000, 0x00200800, 0x00200000, 0x04000802,
    0x04000802, 0x04200002, 0x04200002, 0x00000002, 0x00200002, 0x04000000,
    0x04000800, 0x00200000, 0x04200800, 0x00000802, 0x00200802, 0x04200800,
    0x00000802, 0x04000002, 0x04200802, 0x04200000, 0x00200800, 0x00000000,
    0x00000002, 0x04200802, 0x00000000, 0x00200802, 0x04200000, 0x00000800,
    0x04000002, 0x04000800, 0x00000800, 0x00200002};

static const uint32_t SB8[64] = {
    0x10001040, 0x00001000, 0x00040000, 0x10041040, 0x10000000, 0x10001040,
    0x00000040, 0x10000000, 0x00040040, 0x10040000, 0x10041040, 0x00041000,
    0x10041000, 0x00041040, 0x00001000, 0x00000040, 0x10040000, 0x10000040,
    0x10001000, 0x00001040, 0x00041000, 0x00040040, 0x10040040, 0x10041000,
    0x00001040, 0x00000000, 0x00000000, 0x10040040, 0x10000040, 0x10001000,
    0x00041040, 0x00040000, 0x00041040, 0x00040000, 0x10041000, 0x00001000,
    0x00000040, 0x10040040, 0x00001000, 0x00041040, 0x10001000, 0x00000040,
    0x10000040, 0x10040000, 0x10040040, 0x10000000, 0x00040000, 0x10001040,
    0x00000000, 0x10041040, 0x00040040, 0x10000040, 0x10040000, 0x10001000,
    0x10001040, 0x00000000, 0x10041040, 0x00041000, 0x00041000, 0x00001040,
    0x00001040, 0x00040040, 0x10000000, 0x10041000};

/*
 * PC1: left and right halves bit-swap
 */
static const uint32_t LHs[16] = {
    0x00000000, 0x00000001, 0x00000100, 0x00000101, 0x00010000, 0x00010001,
    0x00010100, 0x00010101, 0x01000000, 0x01000001, 0x01000100, 0x01000101,
    0x01010000, 0x01010001, 0x01010100, 0x01010101};

static const uint32_t RHs[16] = {
    0x00000000, 0x01000000, 0x00010000, 0x01010000, 0x00000100, 0x01000100,
    0x00010100, 0x01010100, 0x00000001, 0x01000001, 0x00010001, 0x01010001,
    0x00000101, 0x01000101, 0x00010101, 0x01010101,
};

/*
 * Initial Permutation macro
 */
#define DES_IP(X, Y)                             \
    {                                            \
        T = ((X >> 4) ^ Y) & 0x0F0F0F0F;         \
        Y ^= T;                                  \
        X ^= (T << 4);                           \
        T = ((X >> 16) ^ Y) & 0x0000FFFF;        \
        Y ^= T;                                  \
        X ^= (T << 16);                          \
        T = ((Y >> 2) ^ X) & 0x33333333;         \
        X ^= T;                                  \
        Y ^= (T << 2);                           \
        T = ((Y >> 8) ^ X) & 0x00FF00FF;         \
        X ^= T;                                  \
        Y ^= (T << 8);                           \
        Y = ((Y << 1) | (Y >> 31)) & 0xFFFFFFFF; \
        T = (X ^ Y) & 0xAAAAAAAA;                \
        Y ^= T;                                  \
        X ^= T;                                  \
        X = ((X << 1) | (X >> 31)) & 0xFFFFFFFF; \
    }

/*
 * Final Permutation macro
 */
#define DES_FP(X, Y)                             \
    {                                            \
        X = ((X << 31) | (X >> 1)) & 0xFFFFFFFF; \
        T = (X ^ Y) & 0xAAAAAAAA;                \
        X ^= T;                                  \
        Y ^= T;                                  \
        Y = ((Y << 31) | (Y >> 1)) & 0xFFFFFFFF; \
        T = ((Y >> 8) ^ X) & 0x00FF00FF;         \
        X ^= T;                                  \
        Y ^= (T << 8);                           \
        T = ((Y >> 2) ^ X) & 0x33333333;         \
        X ^= T;                                  \
        Y ^= (T << 2);                           \
        T = ((X >> 16) ^ Y) & 0x0000FFFF;        \
        Y ^= T;                                  \
        X ^= (T << 16);                          \
        T = ((X >> 4) ^ Y) & 0x0F0F0F0F;         \
        Y ^= T;                                  \
        X ^= (T << 4);                           \
    }

/*
 * DES round macro
 */
#define DES_ROUND(X, Y)                                                     \
    {                                                                       \
        T = *SK++ ^ X;                                                      \
        Y ^= SB8[(T)&0x3F] ^ SB6[(T >> 8) & 0x3F] ^ SB4[(T >> 16) & 0x3F] ^ \
             SB2[(T >> 24) & 0x3F];                                         \
                                                                            \
        T = *SK++ ^ ((X << 28) | (X >> 4));                                 \
        Y ^= SB7[(T)&0x3F] ^ SB5[(T >> 8) & 0x3F] ^ SB3[(T >> 16) & 0x3F] ^ \
             SB1[(T >> 24) & 0x3F];                                         \
    }

#define SWAP(a, b)      \
    {                   \
        uint32_t t = a; \
        a = b;          \
        b = t;          \
    }

void _cc_des_init(_cc_des_t* ctx) {
    bzero(ctx, sizeof(_cc_des_t));
}

void _cc_des3_init(_cc_des3_t* ctx) {
    bzero(ctx, sizeof(_cc_des3_t));
}

static const byte_t odd_parity_table[128] = {
    1,   2,   4,   7,   8,   11,  13,  14,  16,  19,  21,  22,  25,  26,  28,
    31,  32,  35,  37,  38,  41,  42,  44,  47,  49,  50,  52,  55,  56,  59,
    61,  62,  64,  67,  69,  70,  73,  74,  76,  79,  81,  82,  84,  87,  88,
    91,  93,  94,  97,  98,  100, 103, 104, 107, 109, 110, 112, 115, 117, 118,
    121, 122, 124, 127, 128, 131, 133, 134, 137, 138, 140, 143, 145, 146, 148,
    151, 152, 155, 157, 158, 161, 162, 164, 167, 168, 171, 173, 174, 176, 179,
    181, 182, 185, 186, 188, 191, 193, 194, 196, 199, 200, 203, 205, 206, 208,
    211, 213, 214, 217, 218, 220, 223, 224, 227, 229, 230, 233, 234, 236, 239,
    241, 242, 244, 247, 248, 251, 253, 254};

void _cc_des_key_set_parity(byte_t key[_CC_DES_KEY_SIZE_]) {
    int i;

    for (i = 0; i < _CC_DES_KEY_SIZE_; i++) {
        key[i] = odd_parity_table[key[i] / 2];
    }
}

/*
 * Check the given key's parity, returns false on failure, true on SUCCESS
 */
bool_t _cc_des_key_check_key_parity(const byte_t key[_CC_DES_KEY_SIZE_]) {
    int i;

    for (i = 0; i < _CC_DES_KEY_SIZE_; i++) {
        if (key[i] != odd_parity_table[key[i] / 2]) {
            return false;
        }
    }

    return true;
}

/*
 * Table of weak and semi-weak keys
 *
 * Source: http://en.wikipedia.org/wiki/Weak_key
 *
 * Weak:
 * Alternating ones + zeros (0x0101010101010101)
 * Alternating 'F' + 'E' (0xFEFEFEFEFEFEFEFE)
 * '0xE0E0E0E0F1F1F1F1'
 * '0x1F1F1F1F0E0E0E0E'
 *
 * Semi-weak:
 * 0x011F011F010E010E and 0x1F011F010E010E01
 * 0x01E001E001F101F1 and 0xE001E001F101F101
 * 0x01FE01FE01FE01FE and 0xFE01FE01FE01FE01
 * 0x1FE01FE00EF10EF1 and 0xE01FE01FF10EF10E
 * 0x1FFE1FFE0EFE0EFE and 0xFE1FFE1FFE0EFE0E
 * 0xE0FEE0FEF1FEF1FE and 0xFEE0FEE0FEF1FEF1
 *
 */

#define WEAK_KEY_COUNT 16

static const byte_t weak_key_table[WEAK_KEY_COUNT][_CC_DES_KEY_SIZE_] = {
    {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01},
    {0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE},
    {0x1F, 0x1F, 0x1F, 0x1F, 0x0E, 0x0E, 0x0E, 0x0E},
    {0xE0, 0xE0, 0xE0, 0xE0, 0xF1, 0xF1, 0xF1, 0xF1},

    {0x01, 0x1F, 0x01, 0x1F, 0x01, 0x0E, 0x01, 0x0E},
    {0x1F, 0x01, 0x1F, 0x01, 0x0E, 0x01, 0x0E, 0x01},
    {0x01, 0xE0, 0x01, 0xE0, 0x01, 0xF1, 0x01, 0xF1},
    {0xE0, 0x01, 0xE0, 0x01, 0xF1, 0x01, 0xF1, 0x01},
    {0x01, 0xFE, 0x01, 0xFE, 0x01, 0xFE, 0x01, 0xFE},
    {0xFE, 0x01, 0xFE, 0x01, 0xFE, 0x01, 0xFE, 0x01},
    {0x1F, 0xE0, 0x1F, 0xE0, 0x0E, 0xF1, 0x0E, 0xF1},
    {0xE0, 0x1F, 0xE0, 0x1F, 0xF1, 0x0E, 0xF1, 0x0E},
    {0x1F, 0xFE, 0x1F, 0xFE, 0x0E, 0xFE, 0x0E, 0xFE},
    {0xFE, 0x1F, 0xFE, 0x1F, 0xFE, 0x0E, 0xFE, 0x0E},
    {0xE0, 0xFE, 0xE0, 0xFE, 0xF1, 0xFE, 0xF1, 0xFE},
    {0xFE, 0xE0, 0xFE, 0xE0, 0xFE, 0xF1, 0xFE, 0xF1}};

bool_t _cc_des_key_check_weak(const byte_t key[_CC_DES_KEY_SIZE_]) {
    int i;

    for (i = 0; i < WEAK_KEY_COUNT; i++) {
        if (memcmp(weak_key_table[i], key, _CC_DES_KEY_SIZE_) == 0) {
            return true;
        }
    }

    return false;
}

void _cc_des_setkey(uint32_t SK[32], const byte_t key[_CC_DES_KEY_SIZE_]) {
    int i;
    uint32_t X, Y, T;

    GET_UINT32_BE(X, key, 0);
    GET_UINT32_BE(Y, key, 4);

    /*
     * Permuted Choice 1
     */
    T = ((Y >> 4) ^ X) & 0x0F0F0F0F;
    X ^= T;
    Y ^= (T << 4);
    T = ((Y) ^ X) & 0x10101010;
    X ^= T;
    Y ^= (T);

    X = (LHs[(X)&0xF] << 3) | (LHs[(X >> 8) & 0xF] << 2) |
        (LHs[(X >> 16) & 0xF] << 1) | (LHs[(X >> 24) & 0xF]) |
        (LHs[(X >> 5) & 0xF] << 7) | (LHs[(X >> 13) & 0xF] << 6) |
        (LHs[(X >> 21) & 0xF] << 5) | (LHs[(X >> 29) & 0xF] << 4);

    Y = (RHs[(Y >> 1) & 0xF] << 3) | (RHs[(Y >> 9) & 0xF] << 2) |
        (RHs[(Y >> 17) & 0xF] << 1) | (RHs[(Y >> 25) & 0xF]) |
        (RHs[(Y >> 4) & 0xF] << 7) | (RHs[(Y >> 12) & 0xF] << 6) |
        (RHs[(Y >> 20) & 0xF] << 5) | (RHs[(Y >> 28) & 0xF] << 4);

    X &= 0x0FFFFFFF;
    Y &= 0x0FFFFFFF;

    /*
     * calculate subkeys
     */
    for (i = 0; i < 16; i++) {
        if (i < 2 || i == 8 || i == 15) {
            X = ((X << 1) | (X >> 27)) & 0x0FFFFFFF;
            Y = ((Y << 1) | (Y >> 27)) & 0x0FFFFFFF;
        } else {
            X = ((X << 2) | (X >> 26)) & 0x0FFFFFFF;
            Y = ((Y << 2) | (Y >> 26)) & 0x0FFFFFFF;
        }

        *SK++ = ((X << 4) & 0x24000000) | ((X << 28) & 0x10000000) |
                ((X << 14) & 0x08000000) | ((X << 18) & 0x02080000) |
                ((X << 6) & 0x01000000) | ((X << 9) & 0x00200000) |
                ((X >> 1) & 0x00100000) | ((X << 10) & 0x00040000) |
                ((X << 2) & 0x00020000) | ((X >> 10) & 0x00010000) |
                ((Y >> 13) & 0x00002000) | ((Y >> 4) & 0x00001000) |
                ((Y << 6) & 0x00000800) | ((Y >> 1) & 0x00000400) |
                ((Y >> 14) & 0x00000200) | ((Y)&0x00000100) |
                ((Y >> 5) & 0x00000020) | ((Y >> 10) & 0x00000010) |
                ((Y >> 3) & 0x00000008) | ((Y >> 18) & 0x00000004) |
                ((Y >> 26) & 0x00000002) | ((Y >> 24) & 0x00000001);

        *SK++ = ((X << 15) & 0x20000000) | ((X << 17) & 0x10000000) |
                ((X << 10) & 0x08000000) | ((X << 22) & 0x04000000) |
                ((X >> 2) & 0x02000000) | ((X << 1) & 0x01000000) |
                ((X << 16) & 0x00200000) | ((X << 11) & 0x00100000) |
                ((X << 3) & 0x00080000) | ((X >> 6) & 0x00040000) |
                ((X << 15) & 0x00020000) | ((X >> 4) & 0x00010000) |
                ((Y >> 2) & 0x00002000) | ((Y << 8) & 0x00001000) |
                ((Y >> 14) & 0x00000808) | ((Y >> 9) & 0x00000400) |
                ((Y)&0x00000200) | ((Y << 7) & 0x00000100) |
                ((Y >> 7) & 0x00000020) | ((Y >> 3) & 0x00000011) |
                ((Y << 2) & 0x00000004) | ((Y >> 21) & 0x00000002);
    }
}

/*
 * DES key schedule (56-bit, encryption)
 */
void _cc_des_setkey_enc(_cc_des_t *ctx, const byte_t key[_CC_DES_KEY_SIZE_]) {
    _cc_des_setkey(ctx->sk, key);
}

/*
 * DES key schedule (56-bit, decryption)
 */
void _cc_des_setkey_dec(_cc_des_t *ctx, const byte_t key[_CC_DES_KEY_SIZE_]) {
    int i;

    _cc_des_setkey(ctx->sk, key);

    for (i = 0; i < 16; i += 2) {
        SWAP(ctx->sk[i], ctx->sk[30 - i]);
        SWAP(ctx->sk[i + 1], ctx->sk[31 - i]);
    }
}

static void des3_set2key(uint32_t esk[96], uint32_t dsk[96], const byte_t key[_CC_DES_KEY_SIZE_ * 2]) {
    int i;

    _cc_des_setkey(esk, key);
    _cc_des_setkey(dsk + 32, key + 8);

    for (i = 0; i < 32; i += 2) {
        dsk[i] = esk[30 - i];
        dsk[i + 1] = esk[31 - i];

        esk[i + 32] = dsk[62 - i];
        esk[i + 33] = dsk[63 - i];

        esk[i + 64] = esk[i];
        esk[i + 65] = esk[i + 1];

        dsk[i + 64] = dsk[i];
        dsk[i + 65] = dsk[i + 1];
    }
}

/*
 * Triple-DES key schedule (112-bit, encryption)
 */
void _cc_des3_set2key_enc(_cc_des3_t *ctx, const byte_t key[_CC_DES_KEY_SIZE_ * 2]) {
    uint32_t sk[96];

    des3_set2key(ctx->sk, sk, key);
}

/*
 * Triple-DES key schedule (112-bit, decryption)
 */
void _cc_des3_set2key_dec(_cc_des3_t *ctx, const byte_t key[_CC_DES_KEY_SIZE_ * 2]) {
    uint32_t sk[96];

    des3_set2key(sk, ctx->sk, key);
}

static void des3_set3key(uint32_t esk[96], uint32_t dsk[96], const byte_t key[24]) {
    int i;

    _cc_des_setkey(esk, key);
    _cc_des_setkey(dsk + 32, key + 8);
    _cc_des_setkey(esk + 64, key + 16);

    for (i = 0; i < 32; i += 2) {
        dsk[i] = esk[94 - i];
        dsk[i + 1] = esk[95 - i];

        esk[i + 32] = dsk[62 - i];
        esk[i + 33] = dsk[63 - i];

        dsk[i + 64] = esk[30 - i];
        dsk[i + 65] = esk[31 - i];
    }
}

/*
 * Triple-DES key schedule (168-bit, encryption)
 */
void _cc_des3_set3key_enc(_cc_des3_t *ctx, const byte_t key[_CC_DES_KEY_SIZE_ * 3]) {
    uint32_t sk[96];

    des3_set3key(ctx->sk, sk, key);
}

/*
 * Triple-DES key schedule (168-bit, decryption)
 */
void _cc_des3_set3key_dec(_cc_des3_t *ctx, const byte_t key[_CC_DES_KEY_SIZE_ * 3]) {
    uint32_t sk[96];

    des3_set3key(sk, ctx->sk, key);
}

/*
 * DES-ECB block encryption/decryption
 */
#if !defined(_CC_DES_CRYPT_ECB_ALT_)
void _cc_des_crypt_ecb(_cc_des_t *ctx, const byte_t input[8], byte_t output[8]) {
    int i;
    uint32_t X, Y, T, *SK;

    SK = ctx->sk;

    GET_UINT32_BE(X, input, 0);
    GET_UINT32_BE(Y, input, 4);

    DES_IP(X, Y);

    for (i = 0; i < 8; i++) {
        DES_ROUND(Y, X);
        DES_ROUND(X, Y);
    }

    DES_FP(Y, X);

    PUT_UINT32_BE(Y, output, 0);
    PUT_UINT32_BE(X, output, 4);
}
#endif /* !CC_DES_CRYPT_ECB_ALT */

#if defined(_CC_CIPHER_MODE_CBC_)
/*
 * DES-CBC buffer encryption/decryption
 */
int _cc_des_crypt_cbc(_cc_des_t *ctx, int mode, size_t length, byte_t iv[8], const byte_t *input, byte_t *output) {
    int i;
    byte_t temp[8];
    /*length & 7 == length % 8*/
    if (length & 7) {
        return (_CC_ERR_DES_INVALID_INPUT_LENGTH_);
    }

    if (mode == _CC_DES_ENCRYPT_) {
        while (length > 0) {
            for (i = 0; i < 8; i++) {
                output[i] = (byte_t)(input[i] ^ iv[i]);
            }

            _cc_des_crypt_ecb(ctx, output, output);
            memcpy(iv, output, 8);

            input += 8;
            output += 8;
            length -= 8;
        }
    } else { /* _CC_DES_DECRYPT_ */
        while (length > 0) {
            memcpy(temp, input, 8);
            _cc_des_crypt_ecb(ctx, input, output);

            for (i = 0; i < 8; i++) {
                output[i] = (byte_t)(output[i] ^ iv[i]);
            }

            memcpy(iv, temp, 8);

            input += 8;
            output += 8;
            length -= 8;
        }
    }

    return (0);
}
#endif /* _CC_CIPHER_MODE_CBC_ */

/*
 * 3DES-ECB block encryption/decryption
 */
#if !defined(_CC_DES3_CRYPT_ECB_ALT_)
void _cc_des3_crypt_ecb(_cc_des3_t *ctx, const byte_t input[8], byte_t output[8]) {
    int i;
    uint32_t X, Y, T, *SK;

    SK = ctx->sk;

    GET_UINT32_BE(X, input, 0);
    GET_UINT32_BE(Y, input, 4);

    DES_IP(X, Y);

    for (i = 0; i < 8; i++) {
        DES_ROUND(Y, X);
        DES_ROUND(X, Y);
    }

    for (i = 0; i < 8; i++) {
        DES_ROUND(X, Y);
        DES_ROUND(Y, X);
    }

    for (i = 0; i < 8; i++) {
        DES_ROUND(Y, X);
        DES_ROUND(X, Y);
    }

    DES_FP(Y, X);

    PUT_UINT32_BE(Y, output, 0);
    PUT_UINT32_BE(X, output, 4);
}
#endif /* !CC_DES3_CRYPT_ECB_ALT */

#if defined(_CC_CIPHER_MODE_CBC_)
/*
 * 3DES-CBC buffer encryption/decryption
 */
int _cc_des3_crypt_cbc(_cc_des3_t *ctx, int mode, size_t length, byte_t iv[8], const byte_t *input, byte_t *output) {
    int i;
    byte_t temp[8];

    if (length % 8) {
        return (_CC_ERR_DES_INVALID_INPUT_LENGTH_);
    }

    if (mode == _CC_DES_ENCRYPT_) {
        while (length > 0) {
            for (i = 0; i < 8; i++) {
                output[i] = (byte_t)(input[i] ^ iv[i]);
            }

            _cc_des3_crypt_ecb(ctx, output, output);
            memcpy(iv, output, 8);

            input += 8;
            output += 8;
            length -= 8;
        }
    } else { /* _CC_DES_DECRYPT_ */
        while (length > 0) {
            memcpy(temp, input, 8);
            _cc_des3_crypt_ecb(ctx, input, output);

            for (i = 0; i < 8; i++) {
                output[i] = (byte_t)(output[i] ^ iv[i]);
            }

            memcpy(iv, temp, 8);

            input += 8;
            output += 8;
            length -= 8;
        }
    }

    return 0;
}
#endif /* _CC_CIPHER_MODE_CBC_ */