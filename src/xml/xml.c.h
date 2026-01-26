#ifndef _C_CC_XML_C_H_INCLUDED_
#define _C_CC_XML_C_H_INCLUDED_

#include <libcc/xml.h>
#include "../misc/misc.c.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define _XML_NODE_INIT(_NODE, _TYPE)                                    \
    do {                                                                \
        bzero((_NODE), sizeof(_cc_xml_t));                              \
        (_NODE)->type = _TYPE;                                          \
        (_NODE)->name = NULL;                                        \
        _cc_rbtree_cleanup(&(_NODE)->attr);                             \
        _cc_list_cleanup(&(_NODE)->lnk);                       \
        _cc_list_cleanup(&(_NODE)->element.uni_child);         \
    } while (0)

/* Utility to jump whitespace and cr/lf,comments */
bool_t _XML_jump_whitespace(_cc_sbuf_t *const);
bool_t _XML_attr_push(_cc_rbtree_t *, tchar_t *, tchar_t *);

#define _XML_ELEMENT_START_ _T('<')
#define _XML_ELEMENT_END_ _T('>')
#define _XML_ELEMENT_SLASH_ _T('/')

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_XML_C_H_INCLUDED_*/
