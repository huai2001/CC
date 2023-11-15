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
#include "utils.h"
#include <cc/time.h>

/* Render seconds since 1970 as an RFC822 date string.  Return
** a pointer to that string in a static buffer.
*/
tchar_t* httpd_get_rfc822_date(time_t t) {
    struct tm* ptm;
    static tchar_t str_date[128];
    ptm = gmtime(&t);
    _tcsftime(str_date, _cc_countof(str_date), _T("%a, %d %b %Y %H:%M:%S GMT"),
              ptm);
    return str_date;
}
/*
** Parse an RFC822-formatted timestamp as we'd expect from HTTP and return
** a Unix epoch time. <= zero is returned on failure.
*/
time_t httpd_get_rfc822_time(const tchar_t* rfc822_date) {
    struct tm ptm;

    if (rfc822_date == NULL) {
        return 0;
    }

    if (_cc_strptime(rfc822_date, _T("%a, %d %b %Y %H:%M:%S"), &ptm)) {
        return mktime(&ptm);
    }
    return 0;
}

/**/
tchar_t* _skip_left_space(tchar_t* s) {
    while (_cc_isspace(*s)) {
        s++;
    }
    return s;
}

/**/
tchar_t* _http_header_read(tchar_t* z,
                           tchar_t** left,
                           int32_t* len,
                           bool_t (*func)(int)) {
    tchar_t* result;
    if (z == 0) {
        if (left) {
            *left = 0;
        }
        if (len) {
            *len = 0;
        }
        return NULL;
    }

    result = z = _skip_left_space(z);

    while (*z && !func(*z)) {
        z++;
    }

    if (len) {
        *len = (int32_t)(z - result);
    }

    if (*z) {
        *z = 0;
        z = _skip_left_space(++z);
    }

    if (left) {
        *left = z;
    }

    return result;
}

_CC_API_PRIVATE(bool_t) is_end_line(int r) {
    return r == _CC_CR_ || r == _CC_LF_;
}

/**/
tchar_t* _http_header_read_line(tchar_t* z, tchar_t** left, int32_t* len) {
    return _http_header_read(z, left, len, is_end_line);
}