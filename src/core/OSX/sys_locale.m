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
#include <cc/alloc.h>
#import <Foundation/Foundation.h>

_CC_API_PUBLIC(void) _cc_get_preferred_languages(tchar_t *buf, size_t buflen) {
@autoreleasepool {
    NSArray *languages = NSLocale.preferredLanguages;
    size_t numlangs = 0;
    size_t i;

    numlangs = (size_t) [languages count];

    for (i = 0; i < numlangs; i++) {
        NSString *nsstr = [languages objectAtIndex:i];
        size_t len;

        if (nsstr == nil) {
            break;
        }

        [nsstr getCString:buf maxLength:buflen encoding:NSASCIIStringEncoding];
        len = _tcslen(buf);

        if (buflen <= len) {
            *buf = '\0';
            break;
        }

        buf += len;
        buflen -= len;

        if (i < (numlangs - 1)) {
            if (buflen <= 1) {
                break;
            }
            buf[0] = ',';
            buf[1] = '\0';
            buf++;
            buflen--;
        }
    }
}}