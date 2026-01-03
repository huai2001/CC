
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
 * @brief Defines a constant for the number of seconds in a day
 * 
 * This macro defines the total number of seconds in a standard day (24 hours),
 * used for time calculation and conversion scenarios.
 * 
 * Value is 86400 seconds (24 hours × 60 minutes × 60 seconds)
 */
#define _CC_DYAS_PER_SECOND_    86400

/**
 * @brief Number of milliseconds in a second.
 *
 * This is always 1000.
 *
 */
#define _CC_MS_PER_SECOND_   1000

/**
 * @brief Number of microseconds in a second.
 *
 * This is always 1000000.
 *
 */
#define _CC_US_PER_SECOND_   1000000

/**
 * @brief Number of nanoseconds in a second.
 *
 * This is always 1000000000.
 *
 */
#define _CC_NS_PER_SECOND_   1000000000LL

/**
 * @brief Number of nanoseconds in a millisecond.
 *
 * This is always 1000000.
 *
 */
#define _CC_NS_PER_MS_       1000000

/**
 * @brief Number of nanoseconds in a microsecond.
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

/**
 * @brief Suspends execution for a specified time period
 * 
 * @param milliseconds The number of milliseconds to sleep
 * 
 * @note This is a cross-platform sleep function that works on all supported platforms
 * @note The actual sleep duration may be longer than requested due to system scheduling
 */
_CC_API_PUBLIC(void) _cc_sleep(uint32_t milliseconds);

/**
 * @brief Suspends execution for the specified nanoseconds duration
 * 
 * @param nanoseconds The number of nanoseconds to sleep (1-999999999)
 * 
 * @note This is a high-precision sleep function using platform-specific APIs
 * @note The actual sleep duration may be longer than requested due to system scheduling
 */
_CC_API_PUBLIC(void) _cc_nsleep(uint64_t nanoseconds);

/**
 * @brief Retrieve a high-resolution tick counter for interval timing.
 *
 * Returns a platform-dependent, high-resolution tick value suitable for
 * measuring elapsed time.
 *
 * @return Current tick counter (uint64_t).
 */
_CC_API_PUBLIC(uint64_t) _cc_get_ticks(void);

/**
 * @brief Gets the current system timestamp in nanoseconds
 * 
 * Returns the number of nanoseconds elapsed since an arbitrary but fixed point in time.
 * This is typically used for high-precision timing and benchmarking.
 * 
 * @return uint64_t Nanosecond timestamp
 * 
 * @note The exact reference point is platform-dependent but consistent during program execution
 * @note The return value uses uint64_t type to ensure sufficient range for long-running measurements
 */
_CC_API_PUBLIC(uint64_t) _cc_get_ticks_ns(void);

_CC_API_PUBLIC(uint64_t) _cc_query_performance_counter(void);
_CC_API_PUBLIC(uint64_t) _cc_query_performance_frequency(void);
/**
 * @brief Converts calendar date to days since Unix epoch
 *
 * Given a calendar date, returns days since January 1, 1970.
 * Optionally returns the day of week [0-6, 0 is Sunday] and
 * day of year [0-365].
 *
 * @param _year       Year (e.g., 2024)
 * @param month       Month (1-12)
 * @param day         Day of month (1-31)
 * @param day_of_week Output pointer for day of week (can be NULL)
 * @param day_of_year Output pointer for day of year (can be NULL)
 * @return int64_t Days since Unix epoch (1970-01-01)
 */
_CC_API_PUBLIC(int64_t) _cc_civil_to_days(int year, int month, int day, int *day_of_week, int *day_of_year);
/**
 * @brief Gets the number of days in a month
 *
 * Returns the number of days in the specified month and year,
 * accounting for leap years.
 *
 * @param year  Year (e.g., 2024)
 * @param month Month (1-12)
 * @return int Number of days in the month, or -1 on error
 *
 * @note Leap year rule: every 4 years, except every 100 years,
 *       except every 400 years (e.g., 2000 is a leap year, 1900 is not)
 */
_CC_API_PUBLIC(int) _cc_days_in_month(int year, int month);
/**
 * @brief Gets the day of year for a given date
 *
 * Returns the day of year (1-365/366) for the specified calendar date.
 *
 * @param year  Year (e.g., 2024)
 * @param month Month (1-12)
 * @param day   Day of month (1-31)
 * @return int Day of year (1-366), or -1 on error
 */
_CC_API_PUBLIC(int) _cc_day_of_year(int year, int month, int day);
/**
 * @brief Gets the day of week for a given date
 *
 * Returns the day of week for the specified calendar date.
 *
 * @param year  Year (e.g., 2024)
 * @param month Month (1-12)
 * @param day   Day of month (1-31)
 * @return int Day of week [0-6, 0 is Sunday], or -1 on error
 */
_CC_API_PUBLIC(int) _cc_day_of_week(int year, int month, int day);

#ifdef __CC_WIN32_CE__
_CC_API_PUBLIC(time_t) time(time_t*);
_CC_API_PUBLIC(struct tm*) localtime(const time_t*);
#endif

#ifdef __CC_WINDOWS__

#ifdef __CC_MSVC__
struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};

#if __CC_MSVC__ < 1900
struct timespec {
    time_t tv_sec; /* seconds */
    long tv_nsec;  /* nanoseconds */
};
#endif /* __CC_MSVC__ < 1900 */

#endif /* __CC_MSVC__ */

_CC_API_PUBLIC(int) gettimeofday(struct timeval *time_Info, struct timezone *timezone_Info);

#define _cc_gmtime(t, result) gmtime_s((result), (t))
#define _cc_localtime(t, result) localtime_s((result), (t))
#else
#define _cc_gmtime(t, result) gmtime_r((t), (result))
#define _cc_localtime(t, result) localtime_r((t), (result))
#endif /* __CC_WINDOWS__ */
/**/
_CC_API_PUBLIC(const tchar_t *) _cc_strptime(const tchar_t *buf, const tchar_t *fmt, struct tm *tm);
/**/
/**
 * Convert a broken-down date/time to a time_t value.
 *
 * @param year  Full year (e.g. 1970, 2023).
 * @param mon   Month of year, 1-12.
 * @param day   Day of month, 1-31.
 * @param hour  Hour of day, 0-23.
 * @param min   Minute of hour, 0-59.
 * @param sec   Second of minute, 0-60 (60 may be used for a leap second).
 * @param utc   If non-zero, interpret the supplied fields as UTC; if zero, interpret as local time.
 *
 * @return A time_t value representing seconds since the Unix epoch (1970-01-01 00:00:00 UTC),
 *         or (time_t)-1 on error or overflow.
 *
 * Notes:
 * - Implementations may normalize out-of-range fields (for example, month or day values outside
 *   their usual ranges) in a manner similar to the standard mktime/timgm conventions.
 * - Behavior for dates outside the supported range of time_t is undefined and will typically
 *   result in (time_t)-1.
 */
_CC_API_PUBLIC(time_t) _cc_mktime(int32_t year, int32_t mon, int32_t day, int32_t hour, int32_t min, int32_t sec, int32_t utc);
/**
 * @brief Gets the current system timestamp in milliseconds
 * 
 * Returns the number of milliseconds elapsed since the Unix epoch
 * (1970-01-01 00:00:00 UTC). This function is implemented using the
 * POSIX standard gettimeofday() and provides millisecond precision.
 * 
 * @return uint64_t Millisecond timestamp
 * 
 * @note This function is cross-platform compatible and available on
 *       Unix/Linux/macOS/Windows systems
 * @note The return value uses uint64_t type to avoid the 2038 problem
 */
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
