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

#include "generic.c.h"

typedef struct _XML_entity {
    const tchar_t *pattern;
    byte_t length;
    tchar_t value;
} _XML_entity_t;

static _cc_syntax_error_t _cc_global_syntax_error = {nullptr, 0};

#define _XML_NUM_ENTITIES_ 5
static const _XML_entity_t XML_entities[_XML_NUM_ENTITIES_] = {{_T("quot"), 4, _T('\"')},
                                                                    {_T("apos"), 4, _T('\'')},
                                                                    {_T("amp"), 3, _T('&')},
                                                                    {_T("lt"), 2, _T('<')},
                                                                    {_T("gt"), 2, _T('>')}};

_CC_API_PUBLIC(void) _cc_syntax_error(_cc_syntax_error_t *error) {
    _cc_global_syntax_error.content = error->content;
    _cc_global_syntax_error.position = error->position;
}

_CC_API_PUBLIC(const tchar_t*) _cc_get_syntax_error(void) {
    if (_cc_global_syntax_error.position) {
        return (_cc_global_syntax_error.content + _cc_global_syntax_error.position);
    }
    return _cc_global_syntax_error.content;
}

_CC_API_PUBLIC(tchar_t *) _convert_text(tchar_t *alloc_bytes, size_t alloc_length, const tchar_t *input_pointer, const tchar_t *endpos) {
    tchar_t *output_pointer = alloc_bytes;
    /* loop through the string literal */
    while (input_pointer < endpos) {
        if (*input_pointer != '\\') {
            *output_pointer++ = *input_pointer++;
        } else if (*input_pointer == _T('&')) {
            size_t i = 0;
            const tchar_t *p = (input_pointer + 1);
            const _XML_entity_t *entity = nullptr;
            for (i = 0; i < _XML_NUM_ENTITIES_; i++) {
                const _XML_entity_t *tmp = &XML_entities[i];
                if (*tmp->pattern == *p && _tcsnicmp(tmp->pattern, p, tmp->length) == 0) {
                    entity = tmp;
                    break;
                }
            }

            if (entity) {
                *output_pointer++ = entity->value;
                /* +1 skip & */
                input_pointer += entity->length + 1;
            } else {
                *output_pointer++ = *input_pointer++;
            }
        } else {
            /* escape sequence */
            unsigned char sequence_length = 2;
            if ((endpos - input_pointer) < 1) {
                return nullptr;
            }

            switch (input_pointer[1]) {
                case 'b':
                    *output_pointer++ = '\b';
                    break;
                case 'f':
                    *output_pointer++ = '\f';
                    break;
                case 'n':
                    *output_pointer++ = '\n';
                    break;
                case 'r':
                    *output_pointer++ = '\r';
                    break;
                case 't':
                    *output_pointer++ = '\t';
                    break;
                case '\"':
                case '\\':
                case '/':
                    *output_pointer++ = input_pointer[1];
                    break;
                /* UTF-16 literal */
                case 'u':{ 
                    sequence_length = _cc_convert_utf16_literal_to_utf8(&input_pointer, endpos, output_pointer, 
                        alloc_length - (output_pointer - alloc_bytes));
                    if (sequence_length == 0) {
                        /* failed to convert UTF16-literal to UTF-8 */
                        return nullptr;
                    }
                    break;
                }
                default:
                    return nullptr;
            }
            input_pointer += sequence_length;
        }
    }

    /* zero terminate the output */
    *output_pointer = '\0';

    return output_pointer;
}