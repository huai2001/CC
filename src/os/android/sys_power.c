#include <libcc/os/android.h>
#include <libcc/power.h>

_CC_API_PUBLIC(bool_t) _cc_get_sys_power_info(_CC_POWER_STATE_ENUM_ *state, int32_t *seconds, byte_t *percent) {
    int battery;
    int plugged;
    int charged;
    
    Android_JNI_GetPowerInfo(&plugged, &charged, &battery, seconds, percent);

    if (plugged) {
        if (charged) {
            *state = _CC_POWERSTATE_CHARGED_;
        } else if (battery) {
            *state = _CC_POWERSTATE_CHARGING_;
        } else {
            *state = _CC_POWERSTATE_NO_BATTERY_;
        }
    } else {
        *state = _CC_POWERSTATE_ON_BATTERY_;
    }


    return true;
}
