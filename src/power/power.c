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
#include <cc/core.h>
#include <cc/power.h>

bool_t _cc_get_sys_power_info(_CC_POWER_STATE_ENUM_*, int32_t*, byte_t*);

/**/
_CC_POWER_STATE_ENUM_ _cc_get_power_info(int32_t* seconds, byte_t* percent) {
    int32_t _seconds;
    byte_t _percent;
    _CC_POWER_STATE_ENUM_ retval;

    /* Make these never NULL for platform-specific implementations. */
    if (seconds == NULL) {
        seconds = &_seconds;
    }

    if (percent == NULL) {
        percent = &_percent;
    }

    if (_cc_get_sys_power_info(&retval, seconds, percent) == true) {
        return retval;
    }

    /* nothing was definitive. */
    *seconds = -1;
    *percent = -1;

    return _CC_POWERSTATE_UNKNOWN_;
}
