#import <UIKit/UIKit.h>
#include <libcc/power.h>
#include <libcc/os.h>
#include <libcc/time.h>
#ifndef __CC_APPLE_TVOS1__
static uint64_t UIKitLastPowerInfoQuery = 0;
#if 0
/* turn off the battery monitor if it's been more than X ms since last check. */
static const int BATTERY_MONITORING_TIMEOUT = 3000;
_CC_API_PRIVATE(void) UIKit_UpdateBatteryMonitoring(void) {
    if (UIKitLastPowerInfoQuery) {
        if (_CC_TICKS_PASSED(_cc_get_ticks(), UIKitLastPowerInfoQuery + BATTERY_MONITORING_TIMEOUT)) {
            UIDevice *uidev = [UIDevice currentDevice];
            _cc_assert([uidev isBatteryMonitoringEnabled] == YES);
            [uidev setBatteryMonitoringEnabled:NO];
            UIKitLastPowerInfoQuery = 0;
        }
    }
}
#endif
#endif /* !__CC_APPLE_TVOS__ */

_CC_API_PUBLIC(bool_t) _cc_get_sys_power_info(_CC_POWER_STATE_ENUM_ *state, int32_t *seconds, byte_t *percent) {
#if __CC_APPLE_TVOS__
    *state = _CC_POWERSTATE_NO_BATTERY_;
    *seconds = -1;
    *percent = -1;
#else
    @autoreleasepool {
        UIDevice *uidev = [UIDevice currentDevice];

        if (!UIKitLastPowerInfoQuery) {
            _cc_assert([uidev isBatteryMonitoringEnabled] == NO);
            [uidev setBatteryMonitoringEnabled:YES];
        }

        /* UIKit_UpdateBatteryMonitoring() (etc) will check this and disable the battery
         *  monitoring if the app hasn't queried it in the last X seconds.
         *  Apparently monitoring the battery burns battery life.  :)
         *  Apple's docs say not to monitor the battery unless you need it.
         */
        UIKitLastPowerInfoQuery = _cc_get_ticks();

        /* no API to estimate this in UIKit. */
        *seconds = -1;

        switch ([uidev batteryState]) {
            case UIDeviceBatteryStateCharging:
                *state = _CC_POWERSTATE_CHARGING_;
                break;

            case UIDeviceBatteryStateFull:
                *state = _CC_POWERSTATE_CHARGED_;
                break;

            case UIDeviceBatteryStateUnplugged:
                *state = _CC_POWERSTATE_ON_BATTERY_;
                break;

            case UIDeviceBatteryStateUnknown:
            default:
                *state = _CC_POWERSTATE_UNKNOWN_;
                break;
        }

        const float level = [uidev batteryLevel];
        *percent = ( (level < 0.0f) ? -1 : (((int) (level + 0.5f)) * 100) );
    }
#endif /*__CC_APPLE_TVOS__*/

    return true;
}
