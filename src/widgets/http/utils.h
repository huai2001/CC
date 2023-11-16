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
#ifndef _C_CC_HTTP_UTILS_H_INCLUDED_
#define _C_CC_HTTP_UTILS_H_INCLUDED_

#include <cc/alloc.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif
/* Render seconds since 1970 as an RFC822 date string.  Return
** a pointer to that string in a static buffer.
*/
tchar_t* httpd_get_rfc822_date(time_t t);
/*
** Parse an RFC822-formatted timestamp as we'd expect from HTTP and return
** a Unix epoch time. <= zero is returned on failure.
*/
time_t httpd_get_rfc822_time(const tchar_t* rfc822_date);
/**/
tchar_t* _skip_left_space(tchar_t* s);
/**/
tchar_t* _http_header_read(tchar_t* z,
                           tchar_t** left,
                           int32_t* len,
                           bool_t (*func)(int));
/**/
tchar_t* _http_header_read_line(tchar_t* z, tchar_t** left, int32_t* len);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_HTTP_UTILS_H_INCLUDED_*/
