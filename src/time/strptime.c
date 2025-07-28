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
#include <libcc/alloc.h>
#include <libcc/string.h>
#include <libcc/time.h>
#include <libcc/types.h>
#include <stdio.h>
#include <stdlib.h>

#define TM_YEAR_BASE 1900

/*
 * We do not implement alternate representations. However, we always
 * check whether a given modifier is allowed for a certain conversion.
 */
#define ALT_E 0x01
#define ALT_O 0x02
#define LEGAL_ALT(x)                                                                                                   \
    {                                                                                                                  \
        if (alt_format & ~(x)) {                                                                                       \
            return nullptr;                                                                                            \
        }                                                                                                              \
    }

static struct {
    const tchar_t *text;
    int32_t len;
} day[7] = {{_T("Sunday"), 6},   {_T("Monday"), 6}, {_T("Tuesday"), 7}, {_T("Wednesday"), 9},
            {_T("Thursday"), 8}, {_T("Friday"), 6}, {_T("Saturday"), 8}};

#define _SPT_ABDAY_LEN_ 3

static const tchar_t *abday[7] = {_T("Sun"), _T("Mon"), _T("Tue"), _T("Wed"), _T("Thu"), _T("Fri"), _T("Sat")};

static struct {
    const tchar_t *text;
    int32_t len;
} mon[12] = {{_T("January"), 7},   {_T("February"), 8}, {_T("March"), 5},    {_T("April"), 5},
             {_T("May"), 3},       {_T("June"), 4},     {_T("July"), 4},     {_T("August"), 6},
             {_T("September"), 9}, {_T("October"), 7},  {_T("November"), 8}, {_T("December"), 8}};

#define _SPT_ABMON_LEN_ 3
static const tchar_t *abmon[12] = {_T("Jan"), _T("Feb"), _T("Mar"), _T("Apr"), _T("May"), _T("Jun"),
                                   _T("Jul"), _T("Aug"), _T("Sep"), _T("Oct"), _T("Nov"), _T("Dec")};

#define _SPT_AM_LEN_ 2
#define _SPT_PM_LEN_ 2
#define _SPT_AM_ _T("AM")
#define _SPT_PM_ _T("PM")

_CC_API_PRIVATE(int) to_num(const tchar_t **buf, int *dest, int llim, int ulim) {
    int result = 0;

    /* The limit also determines the number of valid digits. */
    int rulim = ulim;

    if (!_CC_ISDIGIT(**buf)) {
        return (0);
    }

    do {
        result *= 10;
        result += *(*buf)++ - '0';
        rulim /= 10;
    } while ((result * 10 <= ulim) && rulim && _CC_ISDIGIT(**buf));

    if (result < llim || result > ulim) {
        return (0);
    }

    *dest = result;
    return (1);
}

_CC_API_PUBLIC(const tchar_t*) _cc_strptime(const tchar_t *buf, const tchar_t *fmt, struct tm *_cp) {
    tchar_t c;
    const tchar_t *bp;
    size_t len = 0;
    int alt_format, i, split_year = 0;

    bp = buf;

    while ((c = *fmt) != '\0') {
        /* Clear `alternate' modifier prior to new conversion. */
        alt_format = 0;

        /* Eat up white-space. */
        if (_istspace(c)) {
            while (_istspace(*bp)) {
                bp++;
            }

            fmt++;
            continue;
        }

        if ((c = *fmt++) != _T('%')) {
            goto literal;
        }

    again:
        switch (c = *fmt++) {
        case _T('%'): /* "%%" is converted to "%". */
        literal:
            if (c != *bp++) {
                return nullptr;
            }
            break;

            /*
             * "Alternative" modifiers. Just set the appropriate flag
             * and start over again.
             */
        case _T('E'): /* "%E?" alternative conversion modifier. */
            LEGAL_ALT(0);
            alt_format |= ALT_E;
            goto again;

        case _T('O'): /* "%O?" alternative conversion modifier. */
            LEGAL_ALT(0);
            alt_format |= ALT_O;
            goto again;

            /*
             * "Complex" conversion rules, implemented through recursion.
             */
        case _T('c'): /* Date and time, using the locale's format. */
            LEGAL_ALT(ALT_E);
            if (!(bp = _cc_strptime(bp, _T("%x %X"), _cp))) {
                return nullptr;
            }
            break;

        case _T('D'): /* The date as "%m/%d/%y". */
            LEGAL_ALT(0);
            if (!(bp = _cc_strptime(bp, _T("%m/%d/%y"), _cp))) {
                return nullptr;
            }
            break;

        case _T('R'): /* The time as "%H:%M". */
            LEGAL_ALT(0);
            if (!(bp = _cc_strptime(bp, _T("%H:%M"), _cp))) {
                return nullptr;
            }
            break;

        case _T('r'): /* The time in 12-hour clock representation. */
            LEGAL_ALT(0);
            if (!(bp = _cc_strptime(bp, _T("%I:%M:%S %p"), _cp))) {
                return nullptr;
            }
            break;

        case _T('T'): /* The time as "%H:%M:%S". */
            LEGAL_ALT(0);
            if (!(bp = _cc_strptime(bp, _T("%H:%M:%S"), _cp))) {
                return nullptr;
            }
            break;

        case _T('X'): /* The time, using the locale's format. */
            LEGAL_ALT(ALT_E);
            if (!(bp = _cc_strptime(bp, _T("%H:%M:%S"), _cp))) {
                return nullptr;
            }
            break;

        case _T('x'): /* The date, using the locale's format. */
            LEGAL_ALT(ALT_E);
            if (!(bp = _cc_strptime(bp, _T("%m/%d/%y"), _cp))) {
                return nullptr;
            }
            break;

            /*
             * "Elementary" conversion rules.
             */
        case _T('A'): /* The day of week, using the locale's form. */
        case _T('a'):
            LEGAL_ALT(0);
            for (i = 0; i < 7; i++) {
                /* Full name. */
                if (_tcsncmp(day[i].text, bp, day[i].len) == 0) {
                    len = day[i].len;
                    break;
                }
                /* Abbreviated name. */
                if (_tcsncmp(abday[i], bp, _SPT_ABDAY_LEN_) == 0) {
                    len = _SPT_ABDAY_LEN_;
                    break;
                }
            }

            /* Nothing matched. */
            if (i == 7) {
                return nullptr;
            }

            _cp->tm_wday = i;
            bp += len;
            break;

        case _T('B'): /* The month, using the locale's form. */
        case _T('b'):
        case _T('h'):
            LEGAL_ALT(0);
            for (i = 0; i < 12; i++) {
                /* Full name. */
                if (_tcsncmp(mon[i].text, bp, mon[i].len) == 0) {
                    len = mon[i].len;
                    break;
                }
                /* Abbreviated name. */
                if (_tcsncmp(abmon[i], bp, _SPT_ABMON_LEN_) == 0) {
                    len = _SPT_ABMON_LEN_;
                    break;
                }
            }
            /* Nothing matched. */
            if (i == 12) {
                return nullptr;
            }

            _cp->tm_mon = i;
            bp += len;
            break;

        case _T('C'): /* The century number. */
            LEGAL_ALT(ALT_E);
            if (!(to_num(&bp, &i, 0, 99))) {
                return nullptr;
            }

            if (split_year) {
                _cp->tm_year = (_cp->tm_year % 100) + (i * 100);
            } else {
                _cp->tm_year = i * 100;
                split_year = 1;
            }
            break;

        case _T('d'): /* The day of month. */
        case _T('e'):
            LEGAL_ALT(ALT_O);
            if (!(to_num(&bp, &_cp->tm_mday, 1, 31))) {
                return nullptr;
            }
            break;

        case _T('k'): /* The hour (24-hour clock representation). */
            LEGAL_ALT(0);
            /* FALLTHROUGH */
        case _T('H'):
            LEGAL_ALT(ALT_O);
            if (!(to_num(&bp, &_cp->tm_hour, 0, 23))) {
                return nullptr;
            }
            break;

        case _T('l'): /* The hour (12-hour clock representation). */
            LEGAL_ALT(0);
            /* FALLTHROUGH */
        case _T('I'):
            LEGAL_ALT(ALT_O);
            if (!(to_num(&bp, &_cp->tm_hour, 1, 12))) {
                return nullptr;
            }
            if (_cp->tm_hour == 12) {
                _cp->tm_hour = 0;
            }
            break;

        case _T('j'): /* The day of year. */
            LEGAL_ALT(0);
            if (!(to_num(&bp, &i, 1, 366))) {
                return nullptr;
            }
            _cp->tm_yday = i - 1;
            break;

        case _T('M'): /* The minute. */
            LEGAL_ALT(ALT_O);
            if (!(to_num(&bp, &_cp->tm_min, 0, 59))) {
                return nullptr;
            }
            break;

        case _T('m'): /* The month. */
            LEGAL_ALT(ALT_O);
            if (!(to_num(&bp, &i, 1, 12))) {
                return nullptr;
            }
            _cp->tm_mon = i - 1;
            break;

        case _T('p'): /* The locale's equivalent of AM/PM. */
            LEGAL_ALT(0);
            /* AM? */
            if (_tcscmp(_SPT_AM_, bp) == 0) {
                if (_cp->tm_hour > 11) {
                    return nullptr;
                }

                bp += _SPT_AM_LEN_;
                break;
            }
            /* PM? */
            else if (_tcscmp(_SPT_PM_, bp) == 0) {
                if (_cp->tm_hour > 11) {
                    return nullptr;
                }

                _cp->tm_hour += 12;
                bp += _SPT_PM_LEN_;
                break;
            }

            /* Nothing matched. */
            return nullptr;

        case _T('S'): /* The seconds. */
            LEGAL_ALT(ALT_O);
            if (!(to_num(&bp, &_cp->tm_sec, 0, 61))) {
                return nullptr;
            }
            break;

        case _T('U'): /* The week of year, beginning on sunday. */
        case _T('W'): /* The week of year, beginning on monday. */
            LEGAL_ALT(ALT_O);
            /*
             * XXX This is bogus, as we can not assume any valid
             * information present in the _cp structure at this
             * point to calculate a real value, so just check the
             * range for now.
             */
            if (!(to_num(&bp, &i, 0, 53))) {
                return nullptr;
            }
            break;

        case _T('w'): /* The day of week, beginning on sunday. */
            LEGAL_ALT(ALT_O);
            if (!(to_num(&bp, &_cp->tm_wday, 0, 6))) {
                return nullptr;
            }
            break;

        case _T('Y'): /* The year. */
            LEGAL_ALT(ALT_E);
            if (!(to_num(&bp, &i, 0, 9999))) {
                return nullptr;
            }

            _cp->tm_year = i - TM_YEAR_BASE;
            break;

        case _T('y'): /* The year within 100 years of the epoch. */
            LEGAL_ALT(ALT_E | ALT_O);
            if (!(to_num(&bp, &i, 0, 99))) {
                return nullptr;
            }

            if (split_year) {
                _cp->tm_year = ((_cp->tm_year / 100) * 100) + i;
                break;
            }

            split_year = 1;
            if (i <= 68) {
                _cp->tm_year = i + 2000 - TM_YEAR_BASE;
            } else {
                _cp->tm_year = i + 1900 - TM_YEAR_BASE;
            }
            break;

            /*
             * Miscellaneous conversions.
             */
        case _T('n'): /* Any kind of white-space. */
        case _T('t'):
            LEGAL_ALT(0);
            while (_istspace(*bp)) {
                bp++;
            }
            break;

        default: /* Unknown/unsupported conversion. */
            return nullptr;
        }
    }

    //_cc_civil_to_days(_cp->tm_year + TM_YEAR_BASE,_cp->tm_mon + 1,_cp->day,&_cp->tm_wday,&_cp->tm_yday);

    /* LINTED functional specification */
    return bp;
}
