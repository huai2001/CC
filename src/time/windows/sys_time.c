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
#include <libcc/socket/windows/sys_socket.h>
#include <libcc/time.h>
#include <libcc/thread.h>

#ifndef __CC_WINRT__
    #include <mmsystem.h>
    #ifdef _CC_MSVC_
        #pragma comment(lib, "winmm.lib")
    #endif
#endif

/**/
_CC_API_PUBLIC(void) _cc_sleep(uint32_t ms) {
    // Sleep(ms);
    /* Sleep() is not publicly available to apps in early versions of WinRT.
     *
     * Visual C++ 2013 Update 4 re-introduced Sleep() for Windows 8.1 and
     * Windows Phone 8.1.
     *
     * Use the compiler version to determine availability.
     *
     * NOTE #1: _MSC_FULL_VER == 180030723 for Visual C++ 2013 Update 3.
     * NOTE #2: Visual C++ 2013, when compiling for Windows 8.0 and
     *    Windows Phone 8.0, uses the Visual C++ 2012 compiler to build
     *    apps and libraries.
     */
#if defined(__CC_WINRT__) && defined(_MSC_FULL_VER) && (_MSC_FULL_VER <= 180030723)
    static HANDLE _sleep_mutex = 0;
    if (!_sleep_mutex) {
        _sleep_mutex = CreateEventEx(0, 0, 0, EVENT_ALL_ACCESS);
        _cc_assert(!_sleep_mutex);
    }
    WaitForSingleObjectEx(_sleep_mutex, ms, false);
#else
    Sleep(ms);
#endif
}

/**/
_CC_API_PUBLIC(void) _cc_nsleep(uint64_t ns) {
    LARGE_INTEGER freq = {0};
    LARGE_INTEGER tc_start = {0};
    LARGE_INTEGER tc_end = {0};
    double counter = 0;

    if (!QueryPerformanceFrequency(&freq)) {
        _cc_logger_error(_T("don't support high precision timers"));
        _cc_sleep(0);
        return;
    }
    
    QueryPerformanceCounter(&tc_start);
    while (true) {
        QueryPerformanceCounter(&tc_end);
        counter = (((tc_end.QuadPart - tc_start.QuadPart) * _CC_US_PER_SECOND_) / (double)freq.QuadPart);
        if (counter >= (double)ns) {
            break;
        }
    }
}

_CC_API_PUBLIC(uint64_t) _cc_query_performance_counter(void) {
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (uint64_t)counter.QuadPart;
}

_CC_API_PUBLIC(uint64_t) _cc_query_performance_frequency(void) {
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);

    return (uint64_t)frequency.QuadPart;
}

typedef VOID(WINAPI *MyGetSystemTimeAsFileTime)(LPFILETIME lpSystemTimeAsFileTime);

static MyGetSystemTimeAsFileTime _getSystemTimeAsFileTimeFunc = nullptr;

_CC_API_PRIVATE(int) getfilesystemtime(struct timeval *tv) {
    FILETIME ft;
    unsigned __int64 ff = 0;
    ULARGE_INTEGER fft;
    
    if (_getSystemTimeAsFileTimeFunc == nullptr) {
        HMODULE hMod = _cc_load_windows_kernel32();
        if (hMod) {
            /* Max possible resolution <1us, win8/server2012 */
            _getSystemTimeAsFileTimeFunc =
                (MyGetSystemTimeAsFileTime)GetProcAddress(hMod, "GetSystemTimePreciseAsFileTime");
        }

        if (!_getSystemTimeAsFileTimeFunc) {
            /* 100ns blocks since 01-Jan-1641 */
            _getSystemTimeAsFileTimeFunc = (MyGetSystemTimeAsFileTime)GetSystemTimeAsFileTime;
        }
    }

    _getSystemTimeAsFileTimeFunc(&ft);

    /*
     * Do not cast a pointer to a FILETIME structure to either a
     * ULARGE_INTEGER* or __int64* value because it can cause alignment faults
     * on 64-bit Windows. via
     * http://technet.microsoft.com/en-us/library/ms724284(v=vs.85).aspx
     */
    fft.HighPart = ft.dwHighDateTime;
    fft.LowPart = ft.dwLowDateTime;
    ff = fft.QuadPart;

    /* convert to microseconds */
    ff /= 10ULL;
    /* convert to unix epoch */
    ff -= 11644473600000000ULL;

    tv->tv_sec = (long)(ff / 1000000ULL);
    tv->tv_usec = (long)(ff % 1000000ULL);

    return 0;
}

/**/
_CC_API_PUBLIC(int) gettimeofday(struct timeval *tp, struct timezone *tzp) {
    /* Get the time, if they want it */
    if (tp != nullptr) {
        getfilesystemtime(tp);
    }
    /* Get the timezone, if they want it */
    if (tzp != nullptr) {
        _tzset();
        tzp->tz_minuteswest = _timezone;
        tzp->tz_dsttime = _daylight;
    }
    /* And return */
    return 0;
}

#ifdef __CC_WIN32_CE__
/*
// Evaluates to true if 'y' is a leap year, otherwise false
// #define IS_LEAP_YEAR(y) ((((y) % 4 == 0) && ((y) % 100 != 0)) || ((y) % 400
== 0))

// The macro below is a reduced version of the above macro.  It is valid for
// years between 1901 and 2099 which easily includes all years representable
// by the current implementation of time_t.
*/
#define IS_LEAP_YEAR(y) (((y)&3) == 0)

#define BASE_DOW 4                         /* 1/1/1970 was a Thursday.*/
#define SECONDS_IN_A_DAY (24L * 60L * 60L) /* Number of seconds in one day. */

/* Month to Year Day conversion array. */
int M2YD[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};

/* Month to Leap Year Day conversion array. */
int M2LYD[] = {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366};

/**/
_CC_API_PRIVATE(void) SafeGetTimeZoneInformation(TIME_ZONE_INFORMATION *ptzi) {
    ZeroMemory(ptzi, sizeof(TIME_ZONE_INFORMATION));

    /* Ask the OS for the standard/daylight rules for the current time zone. */
    if ((GetTimeZoneInformation(ptzi) == 0xFFFFFFFF) || (ptzi->StandardDate.wMonth > 12) ||
        (ptzi->DaylightDate.wMonth > 12)) {
        /* If the OS fails us, we default to the United States' rules. */
        ZeroMemory(ptzi, sizeof(TIME_ZONE_INFORMATION));
        ptzi->StandardDate.wMonth = 10; /* October */
        ptzi->StandardDate.wDay = 5;    /* Last Sunday (DOW == 0) */
        ptzi->StandardDate.wHour = 2;   /* At 2:00 AM */
        ptzi->DaylightBias = -60;       /* One hour difference */
        ptzi->DaylightDate.wMonth = 4;  /* April */
        ptzi->DaylightDate.wDay = 1;    /* First Sunday (DOW == 0) */
        ptzi->DaylightDate.wHour = 2;   /* At 2:00 AM */
    }
}

/******************************************************************************/
_CC_API_PRIVATE(time_t) GetTransitionTimeT(TIME_ZONE_INFORMATION *ptzi, int year, bool_t fStartDST) {
    SYSTEMTIME *pst;
    long daysToYear, yearDay, monthDOW, seconds;
    /*
    // We only handle years within the range that time_t supports.  We need to
    // handle the very end of 1969 since the local time could be up to 13 hours
    // into the previous year.  In this case, our code will actually return a
    // negative value, but it will be compared to another negative value and is
    // handled correctly.  The same goes for the 13 hours past a the max time_t
    // value of 0x7FFFFFFF (in the year 2038).  Again, these values are handled
    // correctly as well.
    */
    if ((year < 1969) || (year > 2038)) {
        return (time_t)0;
    }

    pst = fStartDST ? &ptzi->DaylightDate : &ptzi->StandardDate;
    /*
    // WORD wYear          Year (0000 == 0)
    // WORD wMonth         Month (January == 1)
    // WORD wDayOfWeek     Day of week (Sunday == 0)
    // WORD wDay           Month day (1 - 31)
    // WORD wHour          Hour (0 - 23)
    // WORD wMinute        Minute (0 - 59)
    // WORD wSecond        Second (0 - 59)
    // WORD wMilliseconds  Milliseconds (0 - 999)
    */
    /* Compute the number of days since 1/1/1970 to the beginning of this
     * year.*/
    daysToYear = ((year - 1970) * 365)   /* Tally up previous years.*/
                 + ((year - 1969) >> 2); /* Add few extra for the leap years. */
    /*
    // Compute the number of days since the beginning of this year to the
    // beginning of the month.  We will add to this value to get the actual
    // year day.
    */
    yearDay = IS_LEAP_YEAR(year) ? M2LYD[pst->wMonth - 1] : M2YD[pst->wMonth - 1];

    /* Check for day-in-month format. */
    if (pst->wYear == 0) {
        /* Compute the week day for the first day of the month (Sunday == 0). */
        monthDOW = (daysToYear + yearDay + BASE_DOW) % 7;

        /* Add the day offset of the transition day to the year day. */
        if (monthDOW < pst->wDayOfWeek) {
            yearDay += (pst->wDayOfWeek - monthDOW) + (pst->wDay - 1) * 7;
        } else {
            yearDay += (pst->wDayOfWeek - monthDOW) + pst->wDay * 7;
        }
        /*
        // It is possible that we overshot the month, especially if pst->wDay
        // is 5 (which means the last instance of the day in the month). Check
        // if the year-day has exceeded the month and adjust accordingly.
        */
        if ((pst->wDay == 5) && (yearDay >= (IS_LEAP_YEAR(year) ? M2LYD[pst->wMonth] : M2YD[pst->wMonth]))) {
            yearDay -= 7;
        }
        /* If not day-in-month format, then we assume an absolute date. */
    } else {
        /* Simply add the month day to the current year day. */
        yearDay += pst->wDay - 1;
    }

    /* Tally up all our days, hours, minutes, and seconds since 1970. */
    seconds = ((SECONDS_IN_A_DAY * (daysToYear + yearDay)) + (3600L * (long)pst->wHour) + (60L * (long)pst->wMinute) +
               (long)pst->wSecond);
    /*
    // If we are checking for the end of DST, then we need to add the DST bias
    // since we are in DST when we chack this time stamp.
    */
    if (!fStartDST) {
        seconds += ptzi->DaylightBias * 60L;
    }

    return (time_t)seconds;
}

//******************************************************************************
_CC_API_PRIVATE(bool_t) IsDST(TIME_ZONE_INFORMATION *ptzi, time_t localTime) {
    uint64_t dwl;
    FILETIME ft;
    SYSTEMTIME st;
    time_t timeStart, timeEnd;
    /*
    // If either of the months is 0, then this usually means that the time zone
    // does not use DST.  Unfortunately, Windows CE since it has a bug where it
    // never really fills in these fields with the correct values, so it appears
    // like we are never in DST.  This is supposed to be fixed in future
    releases,
    // so hopefully this code will get some use then.
    */
    if ((ptzi->StandardDate.wMonth == 0) || (ptzi->DaylightDate.wMonth == 0)) {
        return false;
    }
    /*
    // time_t   is a 32-bit value for the seconds since January 1, 1970
    // FILETIME is a 64-bit value for the number of 100-nanosecond intervals
    //          since January 1, 1601
    */
    /* Compute the FILETIME for the given local time. */
    dwl = ((uint64_t)116444736000000000 + ((uint64_t)localTime * (uint64_t)10000000));
    ft = *(FILETIME *)&dwl;

    /* Convert the FILETIME to a SYSTEMTIME. */
    ZeroMemory(&st, sizeof(st));
    FileTimeToSystemTime(&ft, &st);

    /* Get our start and end daylight savings times. */
    timeStart = GetTransitionTimeT(ptzi, (int)st.wYear, true);
    timeEnd = GetTransitionTimeT(ptzi, (int)st.wYear, false);

    /* Check what hemisphere we are in. */
    if (timeStart < timeEnd) {
        /* Northern hemisphere ordering. */
        return ((localTime >= timeStart) && (localTime < timeEnd));

    } else if (timeStart > timeEnd) {
        /* Southern hemisphere ordering. */
        return ((localTime < timeEnd) || (localTime >= timeStart));
    }

    /* If timeStart equals timeEnd then this time zone does not support DST. */
    return false;
}

/**/
_CC_API_PUBLIC(time_t) time(time_t *_tm) {
    union {
        FILETIME ftime;
        time_t itime;
    } ftime;

    SYSTEMTIME stime;

    GetSystemTime(&stime);
    SystemTimeToFileTime(&stime, &ftime.ftime);
    /* Convert 100ns intervals to 1ms intervals */
    ftime.itime /= 10000;
    /* Remove ms portion, which can't be relied on */
    ftime.itime -= (ftime.itime % 1000);

    return (ftime.itime);
}

/**/
_CC_API_PUBLIC(struct tm*) localtime(const time_t *_time) {
    /* Return value for localtime().  Source currently never references */
    /* more than one "tm" at a time, so the single return structure is ok. */
    static struct tm g_tm;
    TIME_ZONE_INFORMATION tzi;
    time_t localTime;
    FILETIME ft;
    SYSTEMTIME st;
    uint64_t dwl;

    ZeroMemory(&g_tm, sizeof(g_tm));

    /* Get our time zone information. */
    SafeGetTimeZoneInformation(&tzi);

    /* Create a time_t that has been corrected for our time zone. */
    localTime = *_time - (tzi.Bias * 60L);

    /* Decide if value is in Daylight Savings Time. */
    if (g_tm.tm_isdst = (int)IsDST(&tzi, localTime)) {
        localTime -= tzi.DaylightBias * 60L; // usually 60 minutes
    } else {
        localTime -= tzi.StandardBias * 60L; // usually  0 minutes
    }
    /*
    // time_t   is a 32-bit value for the seconds since January 1, 1970
    // FILETIME is a 64-bit value for the number of 100-nanosecond intervals
    //          since January 1, 1601
    */
    /* Compute the FILETIME for the given local time. */
    dwl = ((uint64_t)116444736000000000 + ((uint64_t)localTime * (uint64_t)10000000));
    ft = *(FILETIME *)&dwl;

    /* Convert the FILETIME to a SYSTEMTIME. */

    ZeroMemory(&st, sizeof(st));
    FileTimeToSystemTime(&ft, &st);

    /* Finish filling in our "tm" structure. */
    g_tm.tm_sec = (int)st.wSecond;
    g_tm.tm_min = (int)st.wMinute;
    g_tm.tm_hour = (int)st.wHour;
    g_tm.tm_mday = (int)st.wDay;
    g_tm.tm_mon = (int)st.wMonth - 1;
    g_tm.tm_year = (int)st.wYear - 1900;

    return &g_tm;
}

#endif
