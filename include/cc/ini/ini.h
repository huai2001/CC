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
#ifndef _C_CC_INI_H_INCLUDED_
#define _C_CC_INI_H_INCLUDED_

#include "../buf.h"
#include "../rbtree.h"

#if defined(_CC_INI_EXPORT_SHARED_LIBRARY_)
#define _CC_INI_API(t) _CC_API_EXPORT_ t
#elif defined(_CC_INI_IMPORT_SHARED_LIBRARY_)
#define _CC_INI_API(t) _CC_API_IMPORT_ t
#else
#define _CC_INI_API(t) t
#endif

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _cc_int_key {
    tchar_t* name;
    tchar_t* value;

    _cc_rbtree_iterator_t node;
} _cc_ini_key_t;

typedef struct _cc_int_section {
    tchar_t* name;
    _cc_rbtree_t keys;

    _cc_rbtree_iterator_t node;
} _cc_ini_section_t;

typedef struct _cc_ini {
    _cc_rbtree_t root;
} _cc_ini_t;

/**
 * @brief Open INI file
 *
 * @param file_name INI file path
 *
 * @return _cc_xml_t structure
 */
_CC_INI_API(_cc_ini_t*) _cc_open_ini_file(const tchar_t* file_name);

/**
 * @brief Parse INI string
 *
 * @param src INI string
 *
 * @return _cc_ini_t structure
 */
_CC_INI_API(_cc_ini_t*) _cc_parse_ini(const tchar_t* src);

/**
 * @brief Parse INI string
 *
 * @return _cc_xml_t structure
 */
_CC_INI_API(_cc_ini_t*) _cc_ini_parser(_cc_sbuf_tchar_t *const buffer);
/**
 * @brief Destroy ini object
 *
 * @param ctx _cc_ini_t structure
 *
 * @return true if successful or false on error.
 */
_CC_INI_API(bool_t) _cc_destroy_ini(_cc_ini_t** ctx);

/**
 * @brief Print INI Object to buffer
 *
 * @param ctx _cc_ini_t structure
 *
 * @return _cc_buf_t structure
 */
_CC_INI_API(_cc_buf_t*) _cc_print_ini(_cc_ini_t* ctx);

_CC_INI_API(_cc_ini_section_t*)
_cc_ini_find_section(_cc_ini_t* item, const tchar_t* section_name);

_CC_INI_API(const tchar_t*)
_cc_ini_find_string(_cc_ini_section_t* item, const tchar_t* key_name);
_CC_INI_API(const tchar_t*)
_cc_ini_find(_cc_ini_t* item,
             const tchar_t* section_name,
             const tchar_t* key_name);
/**
 * @brief Get XML Parse error message
 *
 * @return XML error message string
 */
_CC_INI_API(const tchar_t*) _cc_ini_error(void);
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_XML_H_INCLUDED_*/
