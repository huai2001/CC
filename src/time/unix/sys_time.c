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
#include <libcc/logger.h>
#include <libcc/time.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

#ifdef CLOCK_MONOTONIC_RAW
#define CC_MONOTONIC_CLOCK CLOCK_MONOTONIC_RAW
#else
#define CC_MONOTONIC_CLOCK CLOCK_MONOTONIC
#endif

#ifdef __CC_APPLE__
#define _CC_HAVE_NANOSLEEP_ 1
#include <mach/mach_time.h>

#ifndef _CC_HAVE_CLOCK_GETTIME_
static mach_timebase_info_data_t mach_base_info;
#endif

#endif
static struct timeval start_tv;

static bool_t has_monotonic_time = false;
static bool_t checked_monotonic_time = false;

static void CheckMonotonicTime(void) {
#ifdef _CC_HAVE_CLOCK_GETTIME_
    struct timespec value;
    if (clock_gettime(CC_MONOTONIC_CLOCK, &value) == 0) {
        has_monotonic_time = true;
    }
#elif defined(__CC_APPLE__)
    if (mach_timebase_info(&mach_base_info) == 0) {
        has_monotonic_time = true;
    }
#endif
    checked_monotonic_time = true;
}

_CC_API_PUBLIC(uint64_t) _cc_query_performance_counter(void) {
    uint64_t ticks;

    if (!checked_monotonic_time) {
        CheckMonotonicTime();
    }

    if (has_monotonic_time) {
#ifdef _CC_HAVE_CLOCK_GETTIME_
        struct timespec now;

        clock_gettime(CC_MONOTONIC_CLOCK, &now);
        ticks = now.tv_sec;
        ticks *= _CC_NS_PER_SECOND_;
        ticks += now.tv_nsec;
#elif defined(__CC_APPLE__)
        ticks = mach_absolute_time();
#else
        _cc_assert(false);
        ticks = 0;
#endif
    } else {
        struct timeval now;

        gettimeofday(&now, nullptr);
        ticks = now.tv_sec;
        ticks *= _CC_US_PER_SECOND_;
        ticks += now.tv_usec;
    }
    return ticks;
}

_CC_API_PUBLIC(uint64_t) _cc_query_performance_frequency(void) {
    if (!checked_monotonic_time) {
        CheckMonotonicTime();
    }
    if (has_monotonic_time) {
#ifdef _CC_HAVE_CLOCK_GETTIME_
        return _CC_NS_PER_SECOND_;
#elif defined(__CC_APPLE__)
        uint64_t freq = mach_base_info.denom;
        freq *= _CC_NS_PER_SECOND_;
        freq /= mach_base_info.numer;
        return freq;
#endif
    }
    return _CC_US_PER_SECOND_;
}

/**/
_CC_API_PUBLIC(void) _cc_sleep(uint32_t ms) {
    _cc_nsleep(_CC_MS_TO_NS(ms));
}

/**/
_CC_API_PUBLIC(void) _cc_nsleep(uint64_t ns) {
    int was_error;
#ifdef _CC_HAVE_NANOSLEEP_
    struct timespec tv, remaining;
#else
    struct timeval tv;
    uint64_t then, now, elapsed;
#endif

    if (_cc_unlikely(ns == 0)) {
        sched_yield();
        return;
    }

    // Set the timeout interval
#ifdef _CC_HAVE_NANOSLEEP_
    remaining.tv_sec = (time_t)(ns / _CC_NS_PER_SECOND_);
    remaining.tv_nsec = (long)(ns % _CC_NS_PER_SECOND_);
#else
    then = _cc_get_ticks_ns();
#endif
    do {
        errno = 0;

#ifdef _CC_HAVE_NANOSLEEP_
        tv.tv_sec = remaining.tv_sec;
        tv.tv_nsec = remaining.tv_nsec;
        was_error = nanosleep(&tv, &remaining);
#else
        // Calculate the time interval left (in case of interrupt)
        now = _cc_get_ticks_ns();
        elapsed = (now - then);
        then = now;
        if (elapsed >= ns) {
            break;
        }
        ns -= elapsed;
        tv.tv_sec = (ns / _CC_NS_PER_SECOND_);
        tv.tv_usec = _CC_NS_TO_US(ns % _CC_NS_PER_SECOND_);

        was_error = select(0, nullptr, nullptr, nullptr, &tv);
#endif // _CC_HAVE_NANOSLEEP_
    } while (was_error && (errno == EINTR));
}