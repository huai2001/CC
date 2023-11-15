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
#include <cc/core.h>
#include <cc/math.h>
#include <cc/rand.h>
#include <cc/time.h>

#define MT_RAND_MT19937 0

#define N (624)                                 /* length of state vector */
#define M (397)                                 /* a period parameter */
#define hi_bit(u) ((u)&0x80000000U)             /* mask all but highest   bit of u */
#define lo_bit(u) ((u)&0x00000001U)             /* mask all but lowest    bit of u */
#define lo_bits(u) ((u)&0x7FFFFFFFU)            /* mask     the highest   bit of u */
#define mix_bits(u, v) (hi_bit(u) | lo_bits(v)) /* move hi bit of u to hi bit of v */

#define twist_v(m, u, v) (m ^ (mix_bits(u, v) >> 1) ^ ((uint32_t)(-(int32_t)(lo_bit(v))) & 0x9908b0dfU))
#define twist_u(m, u, v) (m ^ (mix_bits(u, v) >> 1) ^ ((uint32_t)(-(int32_t)(lo_bit(u))) & 0x9908b0dfU))

#define _CC_GENERATE_SEED() (((long)(time(0) * _cc_getpid())) ^ ((long)(1000000.0 * (long)&mt_rand)))

static struct {
    uint32_t state[N + 1]; /* state vector + 1 extra to not violate ANSI C */
    uint32_t *next;        /* next random value is computed from here */
    int left;              /* can *next++ this many times before reloading */

    bool_t is_seeded; /* Whether _cc_rand() has been seeded */
    long mode;
} mt_rand;

/* */
_CC_API_PRIVATE(void) _mt_initialize(uint32_t seed, uint32_t *state) {
    /* Initialize generator state with seed
       See Knuth TAOCP Vol 2, 3rd Ed, p.106 for multiplier.
       In previous versions, most significant bits (MSBs) of the seed affect
       only MSBs of the state array.  Modified 9 Jan 2002 by Makoto Matsumoto.
     */
    register uint32_t *s = state;
    register uint32_t *r = state;
    register int i = 1;

    *s++ = seed & 0xffffffffU;
    for (; i < N; ++i) {
        *s++ = (1812433253U * (*r ^ (*r >> 30)) + i) & 0xffffffffU;
        r++;
    }
}

_CC_API_PRIVATE(void) _mt_reload(void) {
    /* Generate N new values in state
    Made clearer and faster by Matthew Bellew (matthew.bellew@home.com) */

    register uint32_t *state = mt_rand.state;
    register uint32_t *p = state;
    register int i;

    if (mt_rand.mode == MT_RAND_MT19937) {
        for (i = N - M; i--; ++p) {
            *p = twist_v(p[M], p[0], p[1]);
        }

        for (i = M; --i; ++p) {
            *p = twist_v(p[M - N], p[0], p[1]);
        }

        *p = twist_v(p[M - N], p[0], state[0]);
    } else {
        for (i = N - M; i--; ++p) {
            *p = twist_u(p[M], p[0], p[1]);
        }

        for (i = M; --i; ++p) {
            *p = twist_u(p[M - N], p[0], p[1]);
        }

        *p = twist_u(p[M - N], p[0], state[0]);
    }

    mt_rand.left = N;
    mt_rand.next = state;
}
/* }}} */

_CC_API_PUBLIC(void) _cc_srand(uint32_t seed) {
    /* Seed the generator with a simple uint32 */
    _mt_initialize(seed, mt_rand.state);
    _mt_reload();

    /* Seed only once */
    mt_rand.is_seeded = 1;
}

_CC_API_PUBLIC(uint32_t) _cc_rand(void) {
    /* Pull a 32-bit integer from the generator state
     Every other access function simply transforms the numbers extracted here */
    register uint32_t s1;

    if (_cc_likely(!mt_rand.is_seeded)) {
        _cc_srand((uint32_t)_CC_GENERATE_SEED());
    }

    if (mt_rand.left == 0) {
        _mt_reload();
    }

    --mt_rand.left;

    s1 = *mt_rand.next++;
    s1 ^= (s1 >> 11);
    s1 ^= (s1 << 7) & 0x9d2c5680U;
    s1 ^= (s1 << 15) & 0xefc60000U;

    return (s1 ^ (s1 >> 18));
}

_CC_API_PUBLIC(void) _cc_random_bytes(byte_t *buf, size_t nbytes) {
    size_t i;
    byte_t *cp = buf;
    struct timeval tv;
    unsigned short uuid_rand_seed[3];

    gettimeofday(&tv, 0);
    srand((uint32_t)((_cc_getpid() << 16) ^ getuid() ^ tv.tv_sec ^ tv.tv_usec));

    uuid_rand_seed[0] = getpid() ^ (tv.tv_sec & 0xFFFF);
    uuid_rand_seed[1] = getppid() ^ (tv.tv_usec & 0xFFFF);
    uuid_rand_seed[2] = (tv.tv_sec ^ tv.tv_usec) >> 16;

    /* Crank the random number generator a few times */
    gettimeofday(&tv, 0);
    for (i = (tv.tv_sec ^ tv.tv_usec) & 0x1F; i > 0; i--) {
        rand();
    }

    for (i = 0; i < nbytes; i++) {
        *cp++ ^= (rand() >> 7) & 0xFF;
    }

    for (cp = buf, i = 0; i < nbytes; i++) {
        *cp++ ^= (jrand48(uuid_rand_seed) >> 7) & 0xFF;
    }
}


/**/
_CC_API_PUBLIC(float32_t) _cc_randomf32(float32_t from, float32_t to) {
    return _cc_randf() * (to - from) + from;
}

/**/
_CC_API_PUBLIC(float64_t) _cc_randomf64(float64_t from, float64_t to) {
    return _cc_randf() * (to - from) + from;
}

/**/
_CC_API_PUBLIC(uint32_t) _cc_random32(uint32_t from, uint32_t to) {
    return _cc_rand() * (to - from) + from;
}

/**/
_CC_API_PUBLIC(uint64_t) _cc_random64(uint64_t from, uint64_t to) {
    return _cc_rand() * (to - from) + from;
}

_CC_API_PRIVATE(float64_t) C2P(_cc_prd_t *prd) {
    int32_t i;
    float64_t dCurP = 0.0;
    float64_t dPreSuccessP = 0.0;
    float64_t dPE = 0.0;
    for (i = 1; i <= prd->NMax; ++i) {
        dCurP = _cc_min_float64(1.0, i * prd->C) * (1.0 - dPreSuccessP);
        dPreSuccessP += dCurP;
        dPE += i * dCurP;
    }
    return 1.0 / dPE;
}

/**/
_CC_API_PUBLIC(void) _cc_calculate_prd(_cc_prd_t *prd, float64_t p) {
    float64_t P = p * 1.0 / 100.0;
    float64_t dUp = P;
    float64_t dLow = 0.0;
    float64_t dPLast = 1.0;
    float64_t dPtested;

    while (1) {
        prd->C = (dUp + dLow) / 2.0;
        prd->NMax = (int32_t)ceil(1.0 / prd->C);
        dPtested = C2P(prd);

        if (fabs(dPtested - dPLast) <= 0.000005) {
            break;
        }

        if (dPtested > P) {
            dUp = prd->C;
        } else {
            dLow = prd->C;
        }

        dPLast = dPtested;
    }
}
/*
_CC_API_PRIVATE(void) PRD_Table() {
    PRD *prd;
    int32_t i;
    // 1% - 100%
    for (i = 1; i <= 100; ++i) {
        prd = &probability[i - 1];
        prd->P = i * 1.0 / 100.0;
        P2C(prd);
    }
}

_CC_API_PRIVATE(void) randCurrentCard(byte_t cardData[], byte_t dataCount,
byte_t cardBuffer[]) { byte_t randCount = 0, position = 0, r = 0; do { r =
dataCount - randCount; position = rand() % r; cardBuffer[randCount++] =
cardData[position]; cardData[position] = cardData[r]; } while (randCount <
dataCount);
}
*/
_CC_API_PUBLIC(int32_t) _cc_get_probability(_cc_prd_t *prd, int T) {
    float64_t P, R;

    P = prd->C * T;
    R = rand() * 1.0 / RAND_MAX;
    if (R <= P) {
        return 1;
    }

    if (T == prd->NMax) {
        return 1;
    }
    return 0;
}