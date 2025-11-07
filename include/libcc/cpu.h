
#ifndef _C_CC_CPU_INFO_H_INCLUDED_
#define _C_CC_CPU_INFO_H_INCLUDED_

#include "types.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**/
extern int _cc_cpu_cores;

/**
 *  @brief This function returns the number of CPU cores available.
 */
_CC_API_PUBLIC(int) _cc_get_cpu_cores(void);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _C_CC_CPU_INFO_H_INCLUDED_ */

