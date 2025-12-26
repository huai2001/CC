#ifndef _C_CC_POWER_H_INCLUDED_
#define _C_CC_POWER_H_INCLUDED_

#include "os.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 *  @brief The basic state for the system's power supply.
 */
typedef enum _CC_POWER_STATE_ {
    _CC_POWERSTATE_UNKNOWN_,    /**< cannot determine power status */
    _CC_POWERSTATE_ON_BATTERY_, /**< Not plugged in, running on the battery */
    _CC_POWERSTATE_NO_BATTERY_, /**< Plugged in, no battery available */
    _CC_POWERSTATE_CHARGING_,   /**< Plugged in, charging battery */
    _CC_POWERSTATE_CHARGED_     /**< Plugged in, battery charged */
} _CC_POWER_STATE_ENUM_;

/**
 *  @brief Get the current power supply details.
 *
 *  @param secs Seconds of battery life left. You can pass a NULL here if
 *              you don't care. Will return -1 if we can't determine a
 *              value, or we're not running on a battery.
 *
 *  @param pct Percentage of battery life left, between 0 and 100. You can
 *             pass a NULL here if you don't care. Will return -1 if we
 *             can't determine a value, or we're not running on a battery.
 *
 *  @return The state of the battery (if any).
 */
_CC_API_PUBLIC(_CC_POWER_STATE_ENUM_) _cc_get_power_info(int32_t* secs, byte_t* pct);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_POWER_H_INCLUDED_*/
