#include <libcc/os.h>
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

#if _WIN32_WINNT >= 0x0600
    #include <bcrypt.h>
#ifdef __CC_MSVC__
    #pragma comment(lib, "bcrypt.lib")
#endif

#else
    #include <wincrypt.h> /* CryptAcquireContext, CryptGenRandom */
#endif

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
    #endif
    /* GNU/Hurd defines BSD in sys/param.h which causes problems later */
    #ifndef __GNU__
        #define ARC4RANDOM 1
    #endif
#endif

#ifndef ARC4RANDOM
static int _rand_initialized = 0;
#endif

_CC_API_PUBLIC(void) _cc_srand(uint64_t seed) {
    srand((unsigned int)(seed & 0xFFFFFFFF));
#ifndef ARC4RANDOM
    _rand_initialized = (unsigned int)seed;
#endif
}

_CC_API_PUBLIC(int32_t) _cc_rand(int32_t n) {
#ifdef ARC4RANDOM
    return (int32_t)arc4random_uniform((uint32_t)n);  
#else
    return rand() % n;
#endif
}
_CC_API_PUBLIC(float32_t) _cc_randf(void) {
#ifdef ARC4RANDOM
    return (double)arc4random() / 0x100000000;
#else
    return rand() / (float32_t)RAND_MAX;
#endif
}

#if defined(__CC_WINDOWS__) || defined(__CC_LINUX__)
_CC_API_PRIVATE(void) generic_random_bytes(byte_t *buf, size_t nbytes) {
    byte_t *cp = buf;
    int32_t i;
    int32_t n = ((int32_t) (0x7fffffff & ( ((uint32_t) rand() << 16) ^ ((uint32_t) rand() << 8) ^ ((uint32_t) rand()) )));
    for ( i = 0; i < (int32_t)nbytes; i++) {
        *cp++ ^= (_cc_rand(n + i) >> 7) & 0xFF;
    }
}
#endif

#ifdef __CC_WINDOWS__
_CC_API_PUBLIC(void) _cc_random_bytes(byte_t *buf, size_t nbytes) {
#if _WIN32_WINNT >= 0x0600
    NTSTATUS status = BCryptGenRandom(NULL, buf, (ULONG)nbytes, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if (!BCRYPT_SUCCESS(status)) {
        generic_random_bytes(buf,nbytes);
    }
#else
	HCRYPTPROV ctx;
	BOOL tmp;
	DWORD to_read = 0;
	const size_t MAX_DWORD = 0xFFFFFFFF;

	tmp = CryptAcquireContext(&ctx, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
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
#endif
}

#elif (defined(__CC_LINUX__) || defined(__GNU__)) && (defined(USE_GLIBC) || defined(SYS_getrandom))

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
#elif defined(__CC_LINUX__)
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

_CC_API_PRIVATE(float64_t) C2P(_cc_prd_t *prd) {
    int32_t i;
    float64_t curr = 0.0;
    float64_t upper = 0.0;
    float64_t ptested = 0.0;
    for (i = 1; i <= prd->nmax; ++i) {
        curr = _cc_min_float64(1.0, i * prd->c) * (1.0 - upper);
        upper += curr;
        ptested += i * curr;
    }
    return 1.0 / ptested;
}

/**/
_CC_API_PUBLIC(void) _cc_calculate_prd(_cc_prd_t *prd, float64_t p) {
    float64_t percent = p * 1.0 / 100.0;
    float64_t upper_bound = percent;
    float64_t lower_bound = 0.0;
    float64_t last = 1.0;
    float64_t ptested;

    while (1) {
        prd->c = (upper_bound + lower_bound) / 2.0;
        prd->nmax = (int32_t)ceil(1.0 / prd->c);
        ptested = C2P(prd);

        if (fabs(ptested - last) <= 0.000005) {
            break;
        }

        if (ptested > percent) {
            upper_bound = prd->c;
        } else {
            lower_bound = prd->c;
        }

        last = ptested;
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

    P = prd->c * T;
    R = rand() * 1.0 / RAND_MAX;
    if (R <= P) {
        return 1;
    }

    if (T == prd->nmax) {
        return 1;
    }
    return 0;
}
