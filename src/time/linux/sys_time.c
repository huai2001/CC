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

#include <cc/logger.h>
#include <cc/time.h>
#include <cc/atomic.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

static bool_t ticks_started = false;

/* The first ticks value of the application */
#ifdef _CC_HAVE_CLOCK_GETTIME_
static struct timespec start_tv = {0};

/* Use CLOCK_MONOTONIC_RAW, if available, which is not subject to adjustment by
 * NTP */
#ifdef CLOCK_MONOTONIC_RAW
#define CC_MONOTONIC_CLOCK CLOCK_MONOTONIC_RAW
#else
#define CC_MONOTONIC_CLOCK CLOCK_MONOTONIC
#endif
#else
static struct timeval start_tv = {0};
#endif /* _CC_HAVE_CLOCK_GETTIME_ */

/**/
_CC_API_PRIVATE(void) _tick_init(void) {
    if (ticks_started) {
        return;
    }
    /* Set first ticks value */
#if _CC_HAVE_CLOCK_GETTIME_
    clock_gettime(CC_MONOTONIC_CLOCK, &start_tv);
#else
    gettimeofday(&start_tv, NULL);
#endif
    ticks_started = true;
}

/**/
_CC_API_PRIVATE(void) _tick_quit(void) {
    ticks_started = false;
}

/**/
_CC_API_PUBLIC(void) _cc_sleep(uint32_t ms) {
    struct timespec req;

    if (_cc_unlikely(ms == 0)) {
        sched_yield();
        return;
    }

    req.tv_sec = ms / 1000;
    req.tv_nsec = (ms % 1000) * 1000000;

    while ((-1 == nanosleep(&req, NULL)) && (EINTR == errno));
}

/**/
_CC_API_PUBLIC(void) _cc_nsleep(uint32_t nsec) {
    struct timespec req;

    if (_cc_unlikely(nsec == 0)) {
        sched_yield();
        return;
    }

    req.tv_sec = nsec / 1000000;
    req.tv_nsec = (nsec % 1000000) * 1000;

    while ((-1 == nanosleep(&req, NULL)) && (EINTR == errno));
}

/**/
_CC_API_PUBLIC(uint32_t) _cc_get_ticks(void) {
    uint32_t ticks = 0;
    if (_cc_unlikely(ticks_started == false)) {
        _tick_init();
    }

    {
#if _CC_HAVE_CLOCK_GETTIME_
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        ticks = (uint32_t)(now.tv_sec - start_tv.tv_sec) * 1000 + (now.tv_nsec - start_tv.tv_nsec) / 1000000;
#else
        struct timeval now;
        gettimeofday(&now, NULL);
        ticks = (uint32_t)(now.tv_sec - start_tv.tv_sec) * 1000 + (now.tv_usec - start_tv.tv_usec) / 1000;
#endif
    }
    return ticks;
}