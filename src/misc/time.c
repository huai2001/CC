#include <libcc/time.h>
#include <libcc/logger.h>
#ifdef __CC_WINDOWS__
#include <WinSock.h>
#else
#include <sys/time.h>
#endif
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
_CC_API_PUBLIC(int64_t) _cc_civil_to_days(int _year, int month, int day, int *day_of_week, int *day_of_year) {
    int year = _year - (month <= 2);
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = (unsigned)(year - era * 400);                                  // [0, 399]
    const unsigned doy = (153 * (month > 2 ? month - 3 : month + 9) + 2) / 5 + day - 1; // [0, 365]
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;                         // [0, 146096]
    const int64_t z = (int64_t)(era) * 146097 + (int64_t)(doe) - 719468;

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
