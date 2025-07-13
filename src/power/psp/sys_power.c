/*
 * Copyright libcc.cn@gmail.com. and other libCC contributors.
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
#include <libcc/power.h>

_CC_API_PUBLIC(bool_t) _cc_get_sys_power_info(_CC_POWER_STATE_ENUM_ *state, int32_t *seconds, byte_t *percent) {
    int battery = scePowerIsBatteryExist();
    int plugged = scePowerIsPowerOnline();
    int charging = scePowerIsBatteryCharging();

    *state = _CC_POWERSTATE_UNKNOWN_;
    *seconds = -1;
    *percent = -1;

    if (!battery) {
        *state = _CC_POWERSTATE_NO_BATTERY_;
        *seconds = -1;
        *percent = -1;
    } else if (charging) {
        *state = _CC_POWERSTATE_CHARGING_;
        *percent = (byte_t)scePowerGetBatteryLifePercent();
        *seconds = (int32_t)scePowerGetBatteryLifeTime() * 60;
    } else if (plugged) {
        *state = _CC_POWERSTATE_CHARGED_;
        *percent = (byte_t)scePowerGetBatteryLifePercent();
        *seconds = (int32_t)scePowerGetBatteryLifeTime() * 60;
    } else {
        *state = _CC_POWERSTATE_ON_BATTERY_;
        *percent = (byte_t)scePowerGetBatteryLifePercent();
        *seconds = (int32_t)scePowerGetBatteryLifeTime() * 60;
    }

    return true; /* always the definitive answer on PSP. */
}
