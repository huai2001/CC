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
#ifndef _C_CC_WIDGETS_H_INCLUDED_
#define _C_CC_WIDGETS_H_INCLUDED_

#include "dylib.h"
#ifndef _CC_WIDGETS_EXPORT_SHARED_LIBRARY_
#include "dns.h"
#include "ftp.h"
#include "smtp.h"
#include "http.h"
#include "url_request.h"
#include "ip_locator.h"
#include "socks5.h"
#endif

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif
/**/
_CC_WIDGETS_API(bool_t) _cc_event_writef(_cc_event_t *e, const char_t *fmt, ...);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_WIDGETS_H_INCLUDED_*/
