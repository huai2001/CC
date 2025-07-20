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
#include <libcc/time.h>
#include <libcc/logger.h>
#ifdef __CC_WINDOWS__
#include <WinSock.h>
#else
#include <sys/time.h>
#endif

/* The first (low-resolution) ticks value of the application */
static uint64_t ticks_start = 0;
static bool_t ticks_started = false;

static uint32_t tick_numerator_ns;
static uint32_t tick_denominator_ns;
static uint32_t tick_numerator_ms;
static uint32_t tick_denominator_ms;

static uint32_t CalculateGCD(uint32_t a, uint32_t b) {
    if (b == 0) {
        return a;
    }
    return CalculateGCD(b, (a % b));
}

_CC_API_PRIVATE(void) _tick_init() {
    uint64_t tick_freq;
    uint32_t gcd;

    if (ticks_started) {
        return;
    }

    tick_freq = _cc_query_performance_frequency();
    _cc_assert(tick_freq > 0 && tick_freq <= (uint64_t)0xFFFFFFFFu);

    gcd = CalculateGCD(_CC_NS_PER_SECOND_, (uint32_t)tick_freq);
    tick_numerator_ns = (_CC_NS_PER_SECOND_ / gcd);
    tick_denominator_ns = (uint32_t)(tick_freq / gcd);

    gcd = CalculateGCD(_CC_MS_PER_SECOND_, (uint32_t)tick_freq);
    tick_numerator_ms = (_CC_MS_PER_SECOND_ / gcd);
    tick_denominator_ms = (uint32_t)(tick_freq / gcd);

    ticks_start = _cc_query_performance_counter();
    if (!ticks_start) {
        --ticks_start;
    }
    ticks_started = true;
}

// _CC_API_PRIVATE(void) _tick_quit() {
//     ticks_started = 0;
// }

_CC_API_PUBLIC(uint64_t) _cc_get_ticks_ns(void) {
    uint64_t starting_value, value;
    if (!ticks_started) {
        _tick_init();
    }

    value = _cc_query_performance_counter();
    starting_value = (value - ticks_start);
    value = (starting_value * tick_numerator_ns);
    _cc_assert(value >= starting_value);
    value /= tick_denominator_ns;
    return value;
}

/*
 * The elapsed time is stored as a uint32_t value. Therefore, the time will wrap
 * around to zero if the system is run continuously for 49.7 days.
 */
_CC_API_PUBLIC(uint64_t) _cc_get_ticks(void) {
    uint64_t starting_value, value;
    if (!ticks_started) {
        _tick_init();
    }

    value = _cc_query_performance_counter();
    starting_value = (value - ticks_start);
    value = (starting_value * tick_numerator_ms);
    _cc_assert(value >= starting_value);
    value /= tick_denominator_ms;
    return value;
}

_CC_API_PUBLIC(time_t) _cc_mktime(int32_t year, int32_t mon, int32_t day, int32_t hour, int32_t min, int32_t sec, int32_t utc) {
    /** 1..12 -> 11,12,1..10 */
    if (0 >= (int32_t)(mon -= 2)) {
        /** Puts Feb last since it has leap day */
        mon += 12;
        year -= 1;
    }

    return ((((time_t)(year / 4 - year / 100 + year / 400 + 367 * mon / 12 + day) + year * 365 - 719499) * 24 +
             (hour - utc) /** now have hours */
             ) * 60 +
            min /** now have minutes */
            ) * 60 +
           sec; /** finally seconds */
}

_CC_API_PUBLIC(uint64_t) _cc_timestamp(void) {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return ((uint64_t)tv.tv_sec * 1000) + ((uint64_t)tv.tv_usec / 1000);
}

/* Given a calendar date, returns days since Jan 1 1970, and optionally
 * the day of the week [0-6, 0 is Sunday] and day of the year [0-365].
 */
_CC_API_PUBLIC(int64_t) _cc_civil_to_days(int year, int month, int day, int *day_of_week, int *day_of_year) {
    year -= month <= 2;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = (unsigned)(year - era * 400);                                  // [0, 399]
    const unsigned doy = (153 * (month > 2 ? month - 3 : month + 9) + 2) / 5 + day - 1; // [0, 365]
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;                         // [0, 146096]
    const int64_t z = (int64_t)(era) * 146097 + (int64_t)(doe)-719468;

    if (day_of_week) {
        *day_of_week = (int)(z >= -4 ? (z + 4) % 7 : (z + 5) % 7 + 6);
    }
    if (day_of_year) {
        // This algorithm considers March 1 to be the first day of the year, so offset by Jan + Feb.
        if (doy > 305) {
            // Day 0 is the first day of the year.
            *day_of_year = doy - 306;
        } else {
            const int doy_offset = 59 + (!(year % 4) && ((year % 100) || !(year % 400)));
            *day_of_year = doy + doy_offset;
        }
    }

    return z;
}

_CC_API_PUBLIC(int) _cc_days_in_month(int year, int month) {
    static const int DAYS_IN_MONTH[] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };
    int days;

    if (month < 1 || month > 12) {
        _cc_logger_error(_T("Month out of range [1-12], requested: %i"), month);
        return -1;
    }

    days = DAYS_IN_MONTH[month - 1];

    /* A leap year occurs every 4 years...
     * but not every 100 years...
     * except for every 400 years.
     */
    if (month == 2 && (!(year % 4) && ((year % 100) || !(year % 400)))) {
        ++days;
    }

    return days;
}

_CC_API_PUBLIC(int) _cc_day_of_year(int year, int month, int day) {
    int day_of_year;
    int days;

    if (month < 1 || month > 12) {
        _cc_logger_error(_T("Month out of range [1-12], requested: %i"), month);
        return -1;
    }
    days = _cc_days_in_month(year, month);
    if (day < 1 || day > days) {
        _cc_logger_error(_T("Day out of range [1-%i], requested: %i"), days, month);
        return -1;
    }

    _cc_civil_to_days(year, month, day, nullptr, &day_of_year);
    return day_of_year;
}

_CC_API_PUBLIC(int) _cc_day_of_week(int year, int month, int day) {
    int day_of_week;
    int days;

    if (month < 1 || month > 12) {
        _cc_logger_error(_T("Month out of range [1-12], requested: %i"), month);
        return -1;
    }
    days = _cc_days_in_month(year, month);
    if (day < 1 || day > days) {
        _cc_logger_error(_T("Day out of range [1-%i], requested: %i"), days, month);
        return -1;
    }

    _cc_civil_to_days(year, month, day, &day_of_week, nullptr);
    return day_of_week;
}
