#ifndef _C_CC_INI_C_H_INCLUDED_
#define _C_CC_INI_C_H_INCLUDED_

#include <libcc/alloc.h>
#include <libcc/ini.h>
#include "../misc/misc.c.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

_cc_ini_t* _INI_alloc(int type);
_cc_ini_t* _INI_push(_cc_rbtree_t* root, _cc_sds_t name, int type);
void _INI_free(_cc_ini_t* p);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_INI_C_H_INCLUDED_*/
