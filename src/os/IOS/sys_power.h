#ifndef _C_CC_SYS_UIKIT_POWER_HEAD_FILE_
#define _C_CC_SYS_UIKIT_POWER_HEAD_FILE_

#include <libcc/power.h>
#ifndef __CC_APPLE_TVOS__
void UIKit_UpdateBatteryMonitoring(void);
#endif
bool_t _cc_get_sys_power_info(_CC_POWER_STATE_ENUM_ *, int32_t *, byte_t *);

#endif /* _C_CC_SYS_UIKIT_POWER_HEAD_FILE_ */
