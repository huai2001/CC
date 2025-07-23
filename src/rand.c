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
#include <libcc/core.h>
#include <libcc/math.h>
#include <libcc/time.h>
#include <libcc/rand.h>

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>

/* kFreeBSD */
#if defined(__FreeBSD_kernel__) && defined(__GLIBC__)
    #define GNU_KFREEBSD
#endif

#ifdef __CC_WINDOWS__
    #include <wincrypt.h> /* CryptAcquireContext, CryptGenRandom */
#else
    #include <sys/errno.h>
#endif

#if defined(__CC_LINUX__) || defined(__GNU__) || defined(GNU_KFREEBSD)
    #include <stdint.h>
    #include <stdio.h>
    #include <sys/ioctl.h>

    #if (defined(__CC_LINUX__) || defined(__GNU__)) && defined(__GLIBC__) && ((__GLIBC__ > 2) || (__GLIBC_MINOR__ > 24))
    #   define USE_GLIBC
    #   include <sys/random.h>
    #endif /* (defined(__linux__) || defined(__GNU__)) && defined(__GLIBC__) && ((__GLIBC__ > 2) || (__GLIBC_MINOR__ > 24)) */

    // We need SSIZE_MAX as the maximum read len from /dev/urandom
    #ifndef SSIZE_MAX
    #   define SSIZE_MAX (SIZE_MAX / 2 - 1)
    # endif /* defined(SSIZE_MAX) */

#endif /* defined(__linux__) || defined(__GNU__) || defined(GNU_KFREEBSD) */

#if (defined(__CC_APPLE__) && defined(__MACH__)) || defined(__CC_BSD__)
    /* Dragonfly, FreeBSD, NetBSD, OpenBSD (has arc4random) */
    #include <sys/param.h>
    #if defined(__CC_BSD__)
        #include <stdlib.h>
        #include <bsd/stdlib.h>
    #endif
    /* GNU/Hurd defines BSD in sys/param.h which causes problems later */
    #ifndef __GNU__
        #define ARC4RANDOM 1
    #endif
#endif

static uint64_t _rand_state;
static bool_t _rand_initialized = false;

_CC_API_PUBLIC(void) _cc_srand(uint64_t seed) {
    if (!seed) {
        seed = _cc_query_performance_counter();
    }
    _rand_state = seed;
    _rand_initialized = true;
}

_CC_API_PUBLIC(int32_t) _cc_rand(int32_t n) {

    if (!_rand_initialized) {
        _cc_srand(0);
    }

    return _cc_rand_r(&_rand_state, n);
}
_CC_API_PUBLIC(float32_t) _cc_randf(void) {
    if (!_rand_initialized) {
        _cc_srand(0);
    }

    return _cc_randf_r(&_rand_state);
}

_CC_API_PUBLIC(int32_t) _cc_rand_bits(void) {
    if (!_rand_initialized) {
        _cc_srand(0);
    }

    return _cc_rand_bits_r(&_rand_state);
}

_CC_API_PUBLIC(int32_t) _cc_rand_bits_r(uint64_t *state) {
    if (!state) {
        return 0;
    }
    // The C and A parameters of this LCG have been chosen based on hundreds
    // of core-hours of testing with PractRand and TestU01's Crush.
    // Using a 32-bit A improves performance on 32-bit architectures.
    // C can be any odd number, but < 256 generates smaller code on ARM32
    // These values perform as well as a full 64-bit implementation against
    // Crush and PractRand. Plus, their worst-case performance is better
    // than common 64-bit constants when tested against PractRand using seeds
    // with only a single bit set.

    // We tested all 32-bit and 33-bit A with all C < 256 from a v2 of:
    // Steele GL, Vigna S. Computationally easy, spectrally good multipliers
    // for congruential pseudorandom number generators.
    // Softw Pract Exper. 2022;52(2):443-458. doi: 10.1002/spe.3030
    // https://arxiv.org/abs/2001.05304v2

    *state = *state * 0xff1cd035ul + 0x05;

    // Only return top 32 bits because they have a longer period
    return (int32_t)(*state >> 32);
}

_CC_API_PUBLIC(int32_t) _cc_rand_r(uint64_t *state, int32_t n) {
    int64_t val;
    // Algorithm: get 32 bits from _cc_rand_bits() and treat it as a 0.32 bit
    // fixed point number. Multiply by the 31.0 bit n to get a 31.32 bit
    // result. Shift right by 32 to get the 31 bit integer that we want.

    if (n < 0) {
        // The algorithm looks like it works for numbers < 0 but it has an
        // infinitesimal chance of returning a value out of range.
        // Returning -_cc_rand(abs(n)) blows up at INT_MIN instead.
        // It's easier to just say no.
        return 0;
    }

    // On 32-bit arch, the compiler will optimize to a single 32-bit multiply
    val = (uint64_t)_cc_rand_bits_r(state) * n;
    return (int32_t)(val >> 32);
}

_CC_API_PUBLIC(float32_t) _cc_randf_r(uint64_t *state) {
    // Note: its using 24 bits because float has 23 bits significand + 1 implicit bit
    return (_cc_rand_bits_r(state) >> (32 - 24)) * 0x1p-24f;
}

#define _random()                                                          \
    ((long) (0x7fffffff & ( ((uint32_t) rand() << 16)                      \
                          ^ ((uint32_t) rand() << 8)                       \
                          ^ ((uint32_t) rand()) )))

_CC_API_PRIVATE(void) generic_random_bytes(byte_t *buf, size_t nbytes) {
    byte_t *cp = buf;
    size_t i;
    int32_t n = _random();
    for ( i = 0; i < nbytes; i++) {
        *cp++ ^= (_cc_rand(n) >> 7) & 0xFF;
    }
}

#ifdef __CC_WINDOWS__
_CC_API_PUBLIC(void) _cc_random_bytes(byte_t *buf, size_t nbytes) {
	HCRYPTPROV ctx;
	BOOL tmp;
	DWORD to_read = 0;
	const size_t MAX_DWORD = 0xFFFFFFFF;

	tmp = CryptAcquireContext(&ctx, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
	if (tmp == FALSE) {
        generic_random_bytes(buf,nbytes);
        return;
    }

	while (nbytes > 0) {
		to_read = (DWORD)(nbytes < MAX_DWORD ? nbytes : MAX_DWORD);
		tmp = CryptGenRandom(ctx, to_read, (BYTE*)buf);
		if (tmp == FALSE) {
            break;
        }
		buf = buf + to_read;
		nbytes -= to_read;
	}

	tmp = CryptReleaseContext(ctx, 0);
	if (tmp == FALSE) {
        generic_random_bytes(buf,nbytes);
    }
}
#endif /* defined(_WIN32) */

#if (defined(__CC_LINUX__) || defined(__GNU__)) && (defined(USE_GLIBC) || defined(SYS_getrandom))
#if defined(USE_GLIBC)
// getrandom is declared in glibc.
#elif defined(SYS_getrandom)
_CC_API_PRIVATE(ssize_t) getrandom(void *buf, size_t nbytes, unsigned int flags) {
	return syscall(SYS_getrandom, buf, buflen, flags);
}
#endif

_CC_API_PUBLIC(void) _cc_random_bytes(byte_t *buf, size_t nbytes) {
	/* I have thought about using a separate PRF, seeded by getrandom, but
	 * it turns out that the performance of getrandom is good enough
	 * (250 MB/s on my laptop).
	 */
	size_t offset = 0, chunk;
	int ret;
	while (nbytes > 0) {
		/* getrandom does not allow chunks larger than 33554431 */
		chunk = nbytes <= 33554431 ? nbytes : 33554431;
		do {
			ret = getrandom((char *)buf + offset, chunk, 0);
		} while (ret == -1 && errno == EINTR);

		if (ret < 0) {
            break;
        }
		offset += ret;
		nbytes -= ret;
	}

	return;
}
/* (defined(__linux__) || defined(__GNU__)) && (defined(USE_GLIBC) || defined(SYS_getrandom)) */
#elif defined(__CC_LINUX__) && !defined(__CC_ANDROID__)
_CC_API_PUBLIC(void) _cc_random_bytes(byte_t *buf, size_t nbytes) {
    int fd;
    size_t offset = 0, count;
    ssize_t tmp;
    do {
        fd = open("/dev/urandom", O_RDONLY);
    } while (fd == -1 && errno == EINTR);

    if (fd == -1) {
        generic_random_bytes(buf,nbytes);
        return;
    }

    while (nbytes > 0) {
        count = nbytes <= SSIZE_MAX ? nbytes : SSIZE_MAX;
        tmp = read(fd, (char *)buf + offset, count);
        if (tmp == -1 && (errno == EAGAIN || errno == EINTR)) {
            continue;
        }
        /* Unrecoverable IO error */
        if (tmp == -1) {
            generic_random_bytes(buf,nbytes);
            break;
        }
        offset += tmp;
        nbytes -= tmp;
    }

    close(fd);
}
#endif /* defined(__linux__) */
#ifdef ARC4RANDOM
_CC_API_PUBLIC(void) _cc_random_bytes(byte_t *buf, size_t nbytes) {
	arc4random_buf(buf, nbytes);
}
#endif /* defined(ARC4RANDOM) */


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
    int32_t n = _random();
    return _cc_rand(n) * (to - from) + from;
}

/**/
_CC_API_PUBLIC(uint64_t) _cc_random64(uint64_t from, uint64_t to) {
    int32_t n = _random();
    return _cc_rand(n) * (to - from) + from;
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