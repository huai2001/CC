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

_CC_API_PUBLIC(void) _cc_sleep(uint32_t);
_CC_API_PUBLIC(void) _cc_nsleep(uint32_t);
_CC_API_PUBLIC(uint32_t) _cc_get_ticks(void);

#ifdef __CC_WIN32_CE__
_CC_API_PUBLIC(time_t) time(time_t*);
_CC_API_PUBLIC(struct tm*) localtime(const time_t*);
#endif

#ifdef __CC_WINDOWS__
struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};

#if !defined(timespec) && _MSC_VER < 1900
struct timespec {
    time_t tv_sec; /* seconds */
    long tv_nsec;  /* nanoseconds */
};
#endif

_CC_API_PUBLIC(int)
gettimeofday(struct timeval *time_Info, struct timezone *timezone_Info);

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
 *  uint32_t timeout = _cc_get_ticks() + 100;
 *  while (!CC_TICKS_PASSED(_cc_get_ticks(), timeout)) {
 *      ... do work until timeout has elapsed
 *  }
 */
#define _CC_TICKS_PASSED(A, B) ((int32_t)((B) - (A)) <= 0)

/*
    _cc_wating(100, 100000, {
        int i = read();
        if (i == 1)
            break;
    });
*/
#define _cc_wating(__TIMER, __ELAPSED, __OP)       \
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
