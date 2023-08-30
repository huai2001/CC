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
#include "ini.c.h"

typedef struct {
    const tchar_t* content;
    size_t position;
} _cc_INI_error_t;

static _cc_INI_error_t _cc_global_INI_error = {NULL, 0};

/* Skips spaces and comments as many as possible.*/
bool_t _INI_buf_jump_comments(_cc_sbuf_tchar_t* const buffer) {
    register const tchar_t* p = NULL;
    /*if ((buffer == NULL) || (buffer->content == NULL)) {
        return false;
    }*/
    while (_cc_sbuf_access(buffer)) {
        p = _cc_sbuf_offset(buffer);
        /*Whitespace characters.*/
        if (*p <= 32) {
            if (*p == _T(_CC_LF_)) {
                buffer->line++;
            }

            buffer->offset++;
        } else if (*p == _T(';') || *p == '#') {
            buffer->offset += 1;
            while (_cc_sbuf_access(buffer)) {
                if (*_cc_sbuf_offset(buffer) == _T(_CC_LF_)) {
                    buffer->offset++;
                    buffer->line++;
                    break;
                }
                buffer->offset++;
            }
        } else if (*p == _T('/')) {
            p++;
            /*double-slash comments, to end of line.*/
            if (*p == _T('/')) {
                buffer->offset += 2;
                while (_cc_sbuf_access(buffer)) {
                    if (*_cc_sbuf_offset(buffer) == _T(_CC_LF_)) {
                        buffer->offset++;
                        buffer->line++;
                        break;
                    }
                    buffer->offset++;
                }
                /*multiline comments.*/
            } else if (*p == _T('*')) {
                buffer->offset += 2;
                while (_cc_sbuf_access(buffer)) {
                    p = _cc_sbuf_offset(buffer);
                    if ((*p == _T('*') && *(p + 1) == _T('/'))) {
                        /*skip '*' and '/' */
                        buffer->offset += 2;
                        break;
                    }

                    if (*p == _T(_CC_LF_)) {
                        buffer->line++;
                    }

                    buffer->offset++;
                }
            } else
                break;
        } else
            break;
    }

    return _cc_sbuf_access(buffer);
}

static tchar_t* _INI_read_name(_cc_sbuf_tchar_t* const buffer) {
    tchar_t* output = NULL;
    const tchar_t *start = _cc_sbuf_offset(buffer), *ended = NULL;
    const tchar_t* p = start;

    while (*p && ((size_t)(p - buffer->content) < buffer->length)) {
        if (*p == ']' || *p == '=' || _CC_ISSPACE(*p)) {
            ended = p;
            break;
        }
        p++;
    }

    if (!ended) {
        return NULL;
    }

    output = (tchar_t*)_cc_tcsndup(start, (size_t)(ended - start));
    if (output == NULL) {
        return NULL;
    }

    buffer->offset = (size_t)(p - buffer->content);

    return output;
}

static int _INI_value_endflag(const tchar_t* p,
                                         const tchar_t isquoted) {
    if (isquoted != 0) {
        if (isquoted == *p) {
            return 1;
        }
        return 0;
    }

    if (*p == _T(_CC_CR_) || *p == _T(_CC_LF_)) {
        return 1;
    }

    /* comments */
    if (*p == _T(';') || (*p == _T('/') && *(p + 1) == _T('/')) ||
        (*p == _T('/') && *(p + 1) == _T('*'))) {
        return 2;
    }

    return 0;
}

static tchar_t* _INI_read_string(_cc_sbuf_tchar_t* const buffer) {
    const tchar_t* p = _cc_sbuf_offset(buffer);
    const tchar_t* start;
    const tchar_t* ended;
    size_t alloc_length = 0;
    size_t skipped_bytes = 0;
    tchar_t* output = NULL;
    tchar_t* cps = NULL;
    tchar_t isquoted = *p;
    int32_t convert_bytes = 0;
    int endflag = 0;

    if (isquoted == _T('"') || isquoted == _T('\'')) {
        start = ++p;
    } else {
        isquoted = 0;
        start = p;
    }

    /* calculate approximate size of the output (overestimate) */
    while (*p && ((size_t)(p - buffer->content) < buffer->length)) {
        endflag = _INI_value_endflag(p, isquoted);
        if (endflag != 0) {
            break;
        }
        /* is escape sequence */
        if (*p == _T('\\')) {
            if ((size_t)((p + 1) - buffer->content) >= buffer->length) {
                goto INI_FAIL;
            }
            skipped_bytes++;
            p++;
        }

        p++;
    }

    if (((size_t)(p - buffer->content) >= buffer->length) || endflag == 0) {
        goto INI_FAIL;
    }

    ended = p;
    while (start < ended && _CC_ISSPACE(*(ended - 1))) {
        ended--;
    }
    /* This is at most how much we need for the output */
    alloc_length = (size_t)(ended - start);

    output = (tchar_t*)_cc_malloc(sizeof(tchar_t) * (alloc_length - skipped_bytes));
    cps = output;

    while (start < ended) {
        if (*start != _T('\\')) {
            *cps++ = *start++;
            continue;
        }

        start++;

        switch (*start) {
            case _T('b'):
                *cps++ = _T('\b');
                start++;
                break;
            case _T('f'):
                *cps++ = _T('\f');
                start++;
                break;
            case _T('n'):
                *cps++ = _T('\n');
                start++;
                break;
            case _T('r'):
                *cps++ = _T('\r');
                start++;
                break;
            case _T('t'):
                *cps++ = _T('\t');
                start++;
                break;
            case _T('\"'):
            case _T('\\'):
            case _T('/'): {
                *cps++ = *start++;
            } break;
            /* UTF-16 literal */
            case _T('u'): {
                /* failed to convert UTF16-literal to UTF-8 */
                start++;
                convert_bytes = _cc_convert_utf16_literal_to_utf8(&start, ended, cps, alloc_length);
                if (_cc_unlikely(convert_bytes == 0)) {
                    goto INI_FAIL;
                }
                cps += convert_bytes;
                break;
            }
            default:
                goto INI_FAIL;
                break;
        }
    }

    /* zero terminate the output */
    *cps = 0;

    buffer->offset = (size_t)(p - buffer->content);
    if (isquoted != 0) {
        /* skip \" or \' */
        buffer->offset++;
    }

    return output;

INI_FAIL:
    if (output) {
        buffer->offset = (size_t)(start - buffer->content);
        _cc_free(output);
    }
    return NULL;
}

static bool_t _INI_read(_cc_ini_t* item,
                                   _cc_sbuf_tchar_t* const buffer) {
    /* check if we skipped to the end of the buffer */
    if (!_INI_buf_jump_comments(buffer)) {
        buffer->offset--;
        goto INI_FAIL;
    }

    while (_INI_buf_jump_comments(buffer) && *_cc_sbuf_offset(buffer) == '[') {
        _cc_ini_section_t* section = NULL;
        tchar_t* name = NULL;
        /* skip [ */
        buffer->offset++;

        /* check if we skipped to the end of the buffer */
        if (!_INI_buf_jump_comments(buffer)) {
            buffer->offset--;
            goto INI_FAIL;
        }

        name = _INI_read_name(buffer);
        section = _INI_setion_push(&item->root, name);
        if (section == NULL) {
            _cc_free(name);
            goto INI_FAIL;
        }

        if ((*_cc_sbuf_offset(buffer) != _T(']'))) {
            _cc_free(name);
            goto INI_FAIL;
        }

        /* skip [ */
        buffer->offset++;
        /* check if we skipped to the end of the buffer */
        if (!_INI_buf_jump_comments(buffer)) {
            buffer->offset--;
            goto INI_FAIL;
        }

        while (_cc_sbuf_access(buffer)) {
            tchar_t* name = _INI_read_name(buffer);
            tchar_t* value = NULL;

            /* check if we skipped to the end of the buffer */
            if (!_INI_buf_jump_comments(buffer)) {
                buffer->offset--;
                _cc_free(name);
                goto INI_FAIL;
            }

            if ((*_cc_sbuf_offset(buffer) != _T('='))) {
                _cc_free(name);
                goto INI_FAIL;
            }

            /* skip = */
            buffer->offset++;
            /* check if we skipped to the end of the buffer */
            if (!_INI_buf_jump_comments(buffer)) {
                buffer->offset--;
                _cc_free(name);
                goto INI_FAIL;
            }

            value = _INI_read_string(buffer);

            if (!_INI_key_push(&section->keys, name, value)) {
                _cc_free(name);
                _cc_free(value);
                goto INI_FAIL;
            }

            /* check if we skipped to the end of the buffer */
            _INI_buf_jump_comments(buffer);

            if (*_cc_sbuf_offset(buffer) == _T('[')) {
                break;
            }
        }
    }

    return true;
INI_FAIL:
    return false;
}

_cc_ini_t* _cc_ini_parser(_cc_sbuf_tchar_t* const buffer) {
    _cc_ini_t* item = NULL;
    _cc_INI_error_t local_error;

    local_error.content = NULL;
    local_error.position = 0;

    item = (_cc_ini_t*)_cc_malloc(sizeof(_cc_ini_t));
    if (item == NULL) {
        return 0;
    }
    _CC_RB_INIT_ROOT(&item->root);
    if (_INI_read(item, buffer)) {
        return item;
    }

    local_error.content = buffer->content;
    if (buffer->offset < buffer->length) {
        local_error.position = buffer->offset;
    } else if (buffer->length > 0) {
        local_error.position = buffer->length - 1;
    }

    /*reset error position*/
    _cc_global_INI_error = local_error;

    _cc_destroy_ini(&item);
    return NULL;
}

const tchar_t* _cc_ini_error(void) {
    return (_cc_global_INI_error.content + _cc_global_INI_error.position);
}

_cc_ini_t* _cc_open_ini_file(const tchar_t* file_name) {
    _cc_sbuf_tchar_t buffer;
    _cc_ini_t* item = NULL;

    byte_t* content = NULL;
    size_t offset = 0;

    _cc_buf_t* buf = _cc_load_buf(file_name);

    if (buf == NULL) {
        return NULL;
    }
    content = _cc_buf_bytes(buf);

    /*----BOM----
    EF BB BF = UTF-8
    FE FF 00 = UTF-16, big-endian
    FF FE    = UTF-16, little-endian

    00 00 FE FF = UTF-32, big-endian
    FF FE 00 00 = UTF-32, little-endian
    */

    /*UTF8 BOM */
    if (*content == 0xEF && *(content + 1) == 0xBB && *(content + 2) == 0xBF) {
        offset = 3;
        /*UTF-32 BOM */
    } else if ((*content == 0x00 && *(content + 1) == 0x00 &&
                *(content + 2) == 0xFE && *(content + 3) == 0xFF) ||
               (*content == 0xFF && *(content + 1) == 0xFE &&
                *(content + 2) == 0x00 && *(content + 3) == 0x00)) {
        offset = 4;
        /*UTF-16 BOM big-endian*/
    } else if (*content == 0xFE && *(content + 1) == 0xFF &&
               *(content + 2) == 0x00) {
        offset = 3;
        /*UTF-16 BOM little-endian*/
    } else if (*content == 0xFF && *(content + 1) == 0xFE) {
        offset = 2;
    }

    buffer.content = (tchar_t*)(content + offset);
    buffer.length = (_cc_buf_length(buf) - offset) / sizeof(tchar_t);
    buffer.offset = 0;
    buffer.line = 1;
    buffer.depth = 0;

    item = _cc_ini_parser(&buffer);

    _cc_destroy_buf(&buf);

    return item;
}

_cc_ini_t* _cc_parse_ini(const tchar_t* src) {
    _cc_sbuf_tchar_t buffer;
    buffer.content = src;
    buffer.length = _tcslen(src);
    buffer.offset = 0;
    buffer.line = 1;
    buffer.depth = 0;

    return _cc_ini_parser(&buffer);
}
