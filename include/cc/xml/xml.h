/*
 * Copyright .Qiu<huai2011@163.com>. and other libCC contributors.
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
#ifndef _C_CC_XML_H_INCLUDED_
#define _C_CC_XML_H_INCLUDED_

#include "../buf.h"
#include "../rbtree.h"

#if defined(_CC_XML_EXPORT_SHARED_LIBRARY_)
#define _CC_XML_API(t) _CC_API_EXPORT_ t
#elif defined(_CC_XML_IMPORT_SHARED_LIBRARY_)
#define _CC_XML_API(t) _CC_API_IMPORT_ t
#else
#define _CC_XML_API(t) t
#endif

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cc_XML_attr {
    tchar_t* name;
    tchar_t* value;

    _cc_rbtree_iterator_t node;
} _cc_xml_attr_t;

/**
 * @brief XML Types:
 */
enum _CC_XML_TYPES_ {
    _CC_XML_NULL_ = 0,
    _CC_XML_COMMENT_,
    _CC_XML_CONTEXT_,
    _CC_XML_DOCTYPE_,
    _CC_XML_CHILD_
};

typedef struct _cc_xml_context {
    byte_t cdata;
    tchar_t* text;
} _cc_xml_context_t;

typedef struct _cc_xml {
    /* The type of the item, as above. */
    byte_t type;

    tchar_t* name;

    union {
        tchar_t* uni_comment;
        tchar_t* uni_doctype;
        _cc_xml_context_t uni_context;
        _cc_list_iterator_t uni_child;
    } element;

    _cc_list_iterator_t lnk;
    _cc_rbtree_t attr;
} _cc_xml_t;

/**
 * @brief Open XML file
 *
 * @param file_name XML file path
 *
 * @return _cc_xml_t structure
 */
_CC_XML_API(_cc_xml_t*) _cc_open_xml_file(const tchar_t* file_name);

/**
 * @brief Parse XML string
 *
 * @param src XML string
 *
 * @return _cc_xml_t structure
 */
_CC_XML_API(_cc_xml_t*) _cc_parse_xml(const tchar_t* src);
/**
 * @brief Parse XML string
 *
 * @return _cc_xml_t structure
 */
_CC_XML_API(_cc_xml_t*) _cc_xml_parser(_cc_sbuf_tchar_t *const buffer);
/**
 * @brief Destroy XML object
 *
 * @param ctx _cc_xml_t structure
 *
 * @return true if successful or false on error.
 */
_CC_XML_API(bool_t) _cc_destroy_xml(_cc_xml_t** ctx);

/**
 * @brief Print XML Object to buffer
 *
 * @param ctx _cc_xml_t structure
 *
 * @return _cc_buf_t structure
 */
_CC_XML_API(_cc_buf_t*) _cc_print_xml(_cc_xml_t* ctx);

/**
 * @brief new a XML object
 *
 * @param type type = _CC_XML_TYPES_
 *
 * @return _cc_xml_t structure
 */
_CC_XML_API(_cc_xml_t*) _cc_new_xml_element(byte_t type);
/**
 * @brief Append XML
 *
 * @param ctx _cc_xml_t structure
 * @param child _cc_xml_t structure
 *
 * @return true if successful or false on error.
 */
_CC_XML_API(bool_t) _cc_xml_element_append(_cc_xml_t* ctx, _cc_xml_t* child);
/**
 * @brief Get XML first Object
 *
 * @param ctx _cc_xml_t structure
 *
 * @return _cc_xml_t structure
 */
_CC_XML_API(_cc_xml_t*) _cc_xml_element_first_child(_cc_xml_t* ctx);
/**
 * @brief Get XML next Object
 *
 * @param ctx _cc_xml_t structure
 *
 * @return _cc_xml_t structure
 */
_CC_XML_API(_cc_xml_t*) _cc_xml_element_next_child(_cc_xml_t* ctx);
/**
 * @brief Find XML Object
 *
 * @param ctx _cc_xml_t structure
 * @param item XMLNode Name
 *
 * @return _cc_xml_t structure
 */
_CC_XML_API(_cc_xml_t*) _cc_xml_element_find(_cc_xml_t* ctx, tchar_t* item);
/**
 * @brief Get XML text
 *
 * @param ctx _cc_xml_t structure
 *
 * @return text
 */
_CC_XML_API(const tchar_t*) _cc_xml_element_text(_cc_xml_t* ctx);
/**
 * @brief Get XML attribute value
 *
 * @param ctx _cc_xml_t structure
 * @param keyword Keywords for XML attribute names
 *
 * @return attribute value
 */
_CC_XML_API(const tchar_t*)
_cc_xml_element_attr_find(_cc_xml_t* ctx, const tchar_t* keyword);

/**
 * @brief Set XML attribute value
 *
 * @param ctx _cc_xml_t structure
 * @param keyword XML attribute name
 * @param value XML attribute value
 *
 * @return true if successful or false on error.
 */
_CC_XML_API(bool_t)
_cc_xml_element_set_attr(_cc_xml_t* ctx,
                         const tchar_t* keyword,
                         const tchar_t* fmt,
                         ...);
/**
 * @brief Get XML Parse error message
 *
 * @return XML error message string
 */
_CC_XML_API(const tchar_t*) _cc_xml_error(void);
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_XML_H_INCLUDED_*/
