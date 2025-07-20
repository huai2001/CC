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
#ifndef _C_CC_SYS_UIKIT_POWER_HEAD_FILE_
#define _C_CC_SYS_UIKIT_POWER_HEAD_FILE_

#include <libcc/power.h>
#ifndef __CC_APPLE_TVOS__
void UIKit_UpdateBatteryMonitoring(void);
#endif
bool_t _cc_get_sys_power_info(_CC_POWER_STATE_ENUM_ *, int32_t *, byte_t *);

#endif /* _C_CC_SYS_UIKIT_POWER_HEAD_FILE_ */
