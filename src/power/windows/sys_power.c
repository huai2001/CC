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
#include <libcc/core/windows.h>
#include <libcc/power.h>

/**/
bool_t _cc_get_sys_power_info(_CC_POWER_STATE_ENUM_ *state, int32_t *seconds, byte_t *percent) {
#ifndef __CC_WINRT__
    SYSTEM_POWER_STATUS status;
    bool_t need_details = false;

    /* This API should exist back to Win95. */
    if (!GetSystemPowerStatus(&status)) {
        /* !!! FIXME: push GetLastError() */
        *state = _CC_POWERSTATE_UNKNOWN_;
    } else if (status.BatteryFlag == 0xFF) {
        /* unknown state */
        *state = _CC_POWERSTATE_UNKNOWN_;
    } else if (status.BatteryFlag & (1 << 7)) {
        /* no battery */
        *state = _CC_POWERSTATE_NO_BATTERY_;
    } else if (status.BatteryFlag & (1 << 3)) {
        /* charging */
        *state = _CC_POWERSTATE_CHARGING_;
        need_details = true;
    } else if (status.ACLineStatus == 1) {
        /* on AC, not charging. */
        *state = _CC_POWERSTATE_CHARGED_;
        need_details = true;
    } else {
        /* not on AC. */
        *state = _CC_POWERSTATE_ON_BATTERY_;
        need_details = true;
    }

    *percent = -1;
    *seconds = -1;

    if (need_details) {
        const byte_t pct = (byte_t)status.BatteryLifePercent;
        const int32_t secs = (int32_t)status.BatteryLifeTime;
        /* 255 == unknown */
        if (pct != 0xFF) {
            /* clamp between 0%, 100% */
            *percent = (pct > 100) ? 100 : pct;
        }
        /* ((DWORD)-1) == unknown */
        if (secs != 0xFFFFFFFF) {
            *seconds = secs;
        }
    }
    return true;
#else
    /* Notes:
        - the Win32 function, GetSystemPowerStatus, is not available for use on WinRT
        - Windows Phone 8 has a 'Battery' class, which is documented as available for C++
        - More info on WP8's Battery class can be found at
       http://msdn.microsoft.com/library/windowsphone/develop/jj207231
    */
#endif
}
