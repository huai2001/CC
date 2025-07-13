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
// #include <Carbon/Carbon.h>
#include <IOKit/ps/IOPSKeys.h>
#include <IOKit/ps/IOPowerSources.h>

#include <libcc/power.h>

/* Carbon is so verbose... */
#define STRMATCH(a, b) (CFStringCompare(a, b, 0) == kCFCompareEqualTo)
#define GETVAL(k, v) CFDictionaryGetValueIfPresent(dict, CFSTR(k), (const void **)v)

/* Note that AC power sources also include a laptop battery it is charging. */
_CC_API_PRIVATE(void) checkps(CFDictionaryRef dict, bool_t *have_ac, bool_t *have_battery, bool_t *charging,
                               int32_t *seconds, byte_t *percent) {
    CFStringRef strval;
    CFBooleanRef bval;
    CFNumberRef numval;
    bool_t is_ac = false;
    int32_t maxpct = -1;
    int32_t pct = -1;

    if ((GETVAL(kIOPSIsPresentKey, &bval)) && (bval == kCFBooleanFalse)) {
        /* nothing to see here. */
        return;
    }

    if (!GETVAL(kIOPSPowerSourceStateKey, &strval)) {
        return;
    }

    if (STRMATCH(strval, CFSTR(kIOPSACPowerValue))) {
        is_ac = true;
        *have_ac = is_ac;
    } else if (!STRMATCH(strval, CFSTR(kIOPSBatteryPowerValue))) {
        /* not a battery? */
        return;
    }

    if ((GETVAL(kIOPSIsChargingKey, &bval)) && (bval == kCFBooleanTrue)) {
        *charging = true;
    }

    if (GETVAL(kIOPSMaxCapacityKey, &numval)) {
        SInt32 val = -1;
        CFNumberGetValue(numval, kCFNumberSInt32Type, &val);
        if (val > 0) {
            *have_battery = true;
            maxpct = (int32_t)val;
        }
    }

    if (GETVAL(kIOPSMaxCapacityKey, &numval)) {
        SInt32 val = -1;
        CFNumberGetValue(numval, kCFNumberSInt32Type, &val);
        if (val > 0) {
            *have_battery = true;
            maxpct = (int32_t)val;
        }
    }

    if (GETVAL(kIOPSTimeToEmptyKey, &numval)) {
        SInt32 val = -1;
        CFNumberGetValue(numval, kCFNumberSInt32Type, &val);

        /* Mac OS X reports 0 minutes until empty if you're plugged in. :( */
        if ((val == 0) && (is_ac)) {
            /* !!! FIXME: calc from timeToFull and capacity? */
            val = -1;
        }

        *seconds = (int32_t)val;
        if (*seconds > 0) {
            /* value is in minutes, so convert to seconds. */
            *seconds *= 60;
        }
    }

    if (GETVAL(kIOPSCurrentCapacityKey, &numval)) {
        SInt32 val = -1;
        CFNumberGetValue(numval, kCFNumberSInt32Type, &val);
        pct = (int32_t)val;
    }

    if ((pct > 0) && (maxpct > 0)) {
        pct = (int32_t)((((double)pct) / ((double)maxpct)) * 100.0);
    }

    if (pct > 100) {
        pct = 100;
    }

    *percent = (byte_t)pct;
}

#undef GETVAL
#undef STRMATCH

_CC_API_PUBLIC(bool_t) _cc_get_sys_power_info(_CC_POWER_STATE_ENUM_ *state, int32_t *seconds, byte_t *percent) {
    CFTypeRef blob = IOPSCopyPowerSourcesInfo();

    *seconds = -1;
    *percent = -1;
    *state = _CC_POWERSTATE_UNKNOWN_;

    if (blob != nullptr) {
        CFArrayRef list = IOPSCopyPowerSourcesList(blob);
        if (list != nullptr) {
            /* don't CFRelease() the list items, or dictionaries! */
            bool_t have_ac = false;
            bool_t have_battery = false;
            bool_t charging = false;
            const CFIndex total = CFArrayGetCount(list);
            CFIndex i;
            for (i = 0; i < total; i++) {
                CFTypeRef ps = (CFTypeRef)CFArrayGetValueAtIndex(list, i);
                CFDictionaryRef dict = IOPSGetPowerSourceDescription(blob, ps);
                if (dict != nullptr) {
                    checkps(dict, &have_ac, &have_battery, &charging, seconds, percent);
                }
            }

            if (!have_battery) {
                *state = _CC_POWERSTATE_NO_BATTERY_;
            } else if (charging) {
                *state = _CC_POWERSTATE_CHARGING_;
            } else if (have_ac) {
                *state = _CC_POWERSTATE_CHARGED_;
            } else {
                *state = _CC_POWERSTATE_ON_BATTERY_;
            }
            CFRelease(list);
        }
        CFRelease(blob);
    }
    return true;
}
