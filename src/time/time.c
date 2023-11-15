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
#include <cc/time.h>
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
    gettimeofday(&tv, NULL);
    return ((uint64_t)tv.tv_sec * 1000) + ((uint64_t)tv.tv_usec / 1000);
}