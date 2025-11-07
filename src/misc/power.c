#include <libcc/os.h>
#include <libcc/power.h>

bool_t _cc_get_sys_power_info(_CC_POWER_STATE_ENUM_*, int32_t*, byte_t*);

/**/
_CC_API_PUBLIC(_CC_POWER_STATE_ENUM_) _cc_get_power_info(int32_t* seconds, byte_t* percent) {
    int32_t _seconds;
    byte_t _percent;
    _CC_POWER_STATE_ENUM_ retval;

    /* Make these never nullptr for platform-specific implementations. */
    if (seconds == nullptr) {
        seconds = &_seconds;
    }

    if (percent == nullptr) {
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
