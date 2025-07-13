/*
 * Copyright libcc.cn@gmail.com. and other libCC contributors.
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
#ifndef _C_CC_XML_C_H_INCLUDED_
#define _C_CC_XML_C_H_INCLUDED_

#include <libcc/alloc.h>
#include <libcc/buf.h>
#include <libcc/widgets/xml/xml.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define _XML_NODE_INIT(_NODE, _TYPE)                                    \
    do {                                                                \
        bzero((_NODE), sizeof(_cc_xml_t));                              \
        (_NODE)->type = _TYPE;                                          \
        (_NODE)->name = nullptr;                                           \
        _CC_RB_INIT_ROOT(&(_NODE)->attr);                               \
        _cc_list_iterator_cleanup(&(_NODE)->lnk);                       \
        _cc_list_iterator_cleanup(&(_NODE)->element.uni_child);         \
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
