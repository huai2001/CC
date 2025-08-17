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
#ifndef _C_CC_POWER_H_INCLUDED_
#define _C_CC_POWER_H_INCLUDED_

#include "generic.h"

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
 *  @param secs Seconds of battery life left. You can pass a nullptr here if
 *              you don't care. Will return -1 if we can't determine a
 *              value, or we're not running on a battery.
 *
 *  @param pct Percentage of battery life left, between 0 and 100. You can
 *             pass a nullptr here if you don't care. Will return -1 if we
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
