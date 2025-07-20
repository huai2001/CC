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

#ifndef _C_CC_TIME_H_INCLUDED_
#define _C_CC_TIME_H_INCLUDED_

#include "types.h"

#ifndef __CC_WINDOWS__
#include <sys/time.h>
#endif

#include <time.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Number of milliseconds in a second.
 *
 * This is always 1000.
 *
 */
#define _CC_MS_PER_SECOND_   1000

/**
 * Number of microseconds in a second.
 *
 * This is always 1000000.
 *
 */
#define _CC_US_PER_SECOND_   1000000

/**
 * Number of nanoseconds in a second.
 *
 * This is always 1000000000.
 *
 */
#define _CC_NS_PER_SECOND_   1000000000LL

/**
 * Number of nanoseconds in a millisecond.
 *
 * This is always 1000000.
 *
 */
#define _CC_NS_PER_MS_       1000000

/**
 * Number of nanoseconds in a microsecond.
 *
 * This is always 1000.
 *
 */
#define _CC_NS_PER_US_       1000

#define _CC_SECONDS_TO_NS(S)    (((uint64_t)(S)) * _CC_NS_PER_SECOND_)
#define _CC_NS_TO_SECONDS(NS)   ((NS) / _CC_NS_PER_SECOND_)
#define _CC_MS_TO_NS(MS)        (((uint64_t)(MS)) * _CC_NS_PER_MS_)
#define _CC_NS_TO_MS(NS)        ((NS) / _CC_NS_PER_MS_)
#define _CC_US_TO_NS(US)        (((uint64_t)(US)) * _CC_NS_PER_US_)
#define _CC_NS_TO_US(NS)        ((NS) / _CC_NS_PER_US_)

_CC_API_PUBLIC(void) _cc_sleep(uint32_t);
_CC_API_PUBLIC(void) _cc_nsleep(uint64_t);
_CC_API_PUBLIC(uint64_t) _cc_get_ticks(void);
_CC_API_PUBLIC(uint64_t) _cc_get_ticks_ns(void);
_CC_API_PUBLIC(uint64_t) _cc_query_performance_counter(void);
_CC_API_PUBLIC(uint64_t) _cc_query_performance_frequency(void);

/* Given a calendar date, returns days since Jan 1 1970, and optionally
 * the day of the week [0-6, 0 is Sunday] and day of the year [0-365].
 */
_CC_API_PUBLIC(int64_t) _cc_civil_to_days(int year, int month, int day, int *day_of_week, int *day_of_year);
_CC_API_PUBLIC(int) _cc_days_in_month(int year, int month);
_CC_API_PUBLIC(int) _cc_day_of_year(int year, int month, int day);
_CC_API_PUBLIC(int) _cc_day_of_week(int year, int month, int day);

#ifdef __CC_WIN32_CE__
_CC_API_PUBLIC(time_t) time(time_t*);
_CC_API_PUBLIC(struct tm*) localtime(const time_t*);
#endif

#ifdef __CC_WINDOWS__

#ifdef _CC_MSVC_
struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};

#if _CC_MSVC_ < 1900
struct timespec {
    time_t tv_sec; /* seconds */
    long tv_nsec;  /* nanoseconds */
};
#endif

#endif

_CC_API_PUBLIC(int) gettimeofday(struct timeval *time_Info, struct timezone *timezone_Info);
#endif

_CC_API_PUBLIC(const tchar_t *)
_cc_strptime(const tchar_t *buf, const tchar_t *fmt, struct tm *tm);

_CC_API_PUBLIC(time_t)
_cc_mktime(int32_t year, int32_t mon, int32_t day, int32_t hour, int32_t min, int32_t sec, int32_t utc);

_CC_API_PUBLIC(uint64_t) _cc_timestamp(void);
/**
 * @brief Compare ticks values, and return true if A has passed B
 *
 * e.g. if you want to wait 100 ms, you could do this:
 *  uint64_t timeout = _cc_get_ticks() + 100;
 *  while (!CC_TICKS_PASSED(_cc_get_ticks(), timeout)) {
 *      ... do work until timeout has elapsed
 *  }
 */
#define _CC_TICKS_PASSED(A, B) ((int32_t)((B) - (A)) <= 0)

/*
    _cc_waiting(100, 100000, {
        int i = read();
        if (i == 1)
            break;
    });
*/
#define _cc_waiting(__TIMER, __ELAPSED, __OP)      \
    do {                                           \
        int32_t __elapsed##__TIMER = 0;            \
        while (1) {                                \
            __OP _cc_sleep(__TIMER);               \
            __elapsed##__TIMER += __TIMER;         \
            if (__elapsed##__TIMER >= __ELAPSED) { \
                break;                             \
            }                                      \
        }                                          \
    } while (0)

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _C_CC_TIME_H_INCLUDED_ */
