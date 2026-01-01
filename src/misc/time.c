#include <libcc/time.h>
#include <libcc/logger.h>
#ifdef __CC_WINDOWS__
#include <WinSock.h>
#else
#include <sys/time.h>
#endif
/**
 * @brief Converts calendar date components to Unix timestamp
 *
 * Converts year, month, day, hour, minute, second to seconds since
 * the Unix epoch (1970-01-01 00:00:00 UTC), with optional UTC offset.
 *
 * @param year  Year (e.g., 2024)
 * @param mon   Month (1-12)
 * @param day   Day of month (1-31)
 * @param hour  Hour (0-23)
 * @param min   Minute (0-59)
 * @param sec   Second (0-59)
 * @param utc   UTC offset in hours
 * @return time_t Unix timestamp in seconds
 */
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
_CC_API_PUBLIC(uint64_t) _cc_timestamp(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((uint64_t)tv.tv_sec * 1000) + ((uint64_t)tv.tv_usec / 1000);
}

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

    _cc_civil_to_days(year, month, day, NULL, &day_of_year);
    return day_of_year;
}

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

    _cc_civil_to_days(year, month, day, &day_of_week, NULL);
    return day_of_week;
}
