#ifndef _C_CC_VERSION__H_INCLUDED_
#define _C_CC_VERSION__H_INCLUDED_

#include "os/compiler.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** @name Version Number
 *  Printable format: "%d.%d.%d", MAJOR, MINOR, MICRO
*/
#define _CC_MAJOR_VERSION_    2
#define _CC_MINOR_VERSION_    0
#define _CC_MICRO_VERSION_    0

#define _CC_VERSION_ "2.0.0"

/**
 * This macro turns the version numbers into a numeric value.
 *
 * (1,2,3) becomes 1002003.
 *
 * \param major the major version number.
 * \param minor the minorversion number.
 * \param patch the patch version number.
 *
 */
#define _CC_VERSIONNUM(major, minor, patch) \
    ((major) * 1000000 + (minor) * 1000 + (patch))

_CC_FORCE_INLINE_ int _cc_get_version(void) {
    return _CC_VERSIONNUM(_CC_MAJOR_VERSION_, _CC_MINOR_VERSION_, _CC_MICRO_VERSION_);
}

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_VERSION__H_INCLUDED_*/
