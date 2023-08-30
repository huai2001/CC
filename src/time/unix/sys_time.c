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
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

#ifdef __CC_APPLE__
#define _CC_HAVE_NANOSLEEP_ 1
#include <mach/mach_time.h>
static mach_timebase_info_data_t mach_base_info;
static uint64_t start_mach = 0;
static bool_t has_monotonic_time = false;
#endif
static bool_t ticks_started = false;
static struct timeval start_tv;

/**/
void _cc_sleep(uint32_t ms) {
    int was_error;
    struct timespec elapsed, tv;

    if (_cc_unlikely(ms == 0)) {
        sched_yield();
        return;
    }

    /* Set the timeout interval */
    elapsed.tv_sec = ms / 1000;
    elapsed.tv_nsec = (ms % 1000) * 1000000;

    do {
        errno = 0;
        tv.tv_sec = elapsed.tv_sec;
        tv.tv_nsec = elapsed.tv_nsec;

#if _CC_HAVE_NANOSLEEP_
        was_error = nanosleep(&tv, &elapsed);
#else
        was_error = pselect(0, NULL, NULL, NULL, &tv, NULL);
#endif /* _CC_HAVE_NANOSLEEP_ */
    } while (was_error == -1 && (errno == EINTR));
}

/**/
void _cc_nsleep(uint32_t nsec) {
    int was_error;
    struct timespec elapsed, tv;

    if (_cc_unlikely(nsec == 0)) {
        sched_yield();
        return;
    }

    /* Set the timeout interval */
    elapsed.tv_sec = nsec / 1000000;
    elapsed.tv_nsec = (nsec % 1000000) * 1000;
    do {
        errno = 0;
        tv.tv_sec = elapsed.tv_sec;
        tv.tv_nsec = elapsed.tv_nsec;

#if _CC_HAVE_NANOSLEEP_
        was_error = nanosleep(&tv, &elapsed);
#else
        was_error = pselect(0, NULL, NULL, NULL, &tv, NULL);
#endif /* _CC_HAVE_NANOSLEEP_ */
    } while (was_error == -1 && (errno == EINTR));
}

static void _cc_tick_init(void) {
    if (ticks_started) {
        return;
    }

    /* Set first ticks value */
#ifdef __CC_APPLE__
    if (mach_timebase_info(&mach_base_info) == KERN_SUCCESS) {
        has_monotonic_time = true;
        start_mach = mach_absolute_time();
    } else
#endif
    {
        gettimeofday(&start_tv, NULL);
    }
    ticks_started = true;
}

uint32_t _cc_get_ticks(void) {
    uint32_t ticks;
    if (!ticks_started) {
        _cc_tick_init();
    }

#ifdef __CC_APPLE__
    if (has_monotonic_time) {
        uint64_t now = mach_absolute_time();
        ticks = (uint32_t)((((now - start_mach) * mach_base_info.numer) / mach_base_info.denom) / 1000000);
    } else
#endif
    {
        struct timeval now;
        gettimeofday(&now, NULL);
        ticks = (uint32_t)((now.tv_sec - start_tv.tv_sec) * 1000 + (now.tv_usec - start_tv.tv_usec) / 1000);
    }
    return (ticks);
}
