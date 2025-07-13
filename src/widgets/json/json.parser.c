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
#include "json.c.h"

typedef struct {
    const tchar_t *content;
    size_t position;
} _cc_json_error_t;

static _cc_json_error_t _cc_global_json_error = {nullptr, 0};

_CC_API_PUBLIC(const tchar_t*) _cc_json_error(void) {
    if (_cc_unlikely(_cc_global_json_error.content == nullptr)) {
        return nullptr;
    }
    return (_cc_global_json_error.content + _cc_global_json_error.position);
}

_CC_API_PRIVATE(bool_t) _json_read(_cc_sbuf_t *const buffer, _cc_json_t *item);

_CC_API_PRIVATE(tchar_t*) _json_parser_string(_cc_sbuf_t *const buffer, size_t *length) {
    const tchar_t *p = _cc_sbuf_offset(buffer);
    const tchar_t *start = nullptr;
    size_t alloc_length = 0;
    size_t skipped_bytes = 0;
    tchar_t *output = nullptr;
    tchar_t *cps = nullptr;
    tchar_t quotes = *p;
    int32_t convert_bytes = 0;

    if (_cc_likely(quotes == _T('"') || quotes == _T('\''))) {
        start = ++p;
    } else {
        return nullptr;
    }

    /* calculate approximate size of the output (overestimate) */
    while (*p && ((size_t)(p - buffer->content) < buffer->length) && (*p) != quotes) {
        /* is escape sequence */
        if (*p == _T('\\')) {
            if ((size_t)((p + 1) - buffer->content) >= buffer->length) {
                goto JSON_FAIL;
            }
            skipped_bytes++;
            p++;
        }
        p++;
    }

    if (_cc_unlikely(((size_t)(p - buffer->content) >= buffer->length) || (*p) != quotes)) {
        goto JSON_FAIL;
    }

    /* This is at most how much we need for the output */
    alloc_length = sizeof(tchar_t) * ((size_t)(p - start) - skipped_bytes + 1);
    cps = output = (tchar_t *)_cc_malloc(alloc_length);

    while (start < p) {
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
            convert_bytes = _cc_convert_utf16_literal_to_utf8(&start, p, cps, alloc_length);
            if (_cc_unlikely(convert_bytes == 0)) {
                goto JSON_FAIL;
            }
            cps += convert_bytes;
            break;
        }
        default:
            goto JSON_FAIL;
            break;
        }
    }
    /* zero terminate the output */
    *cps = 0;
    if (length) {
        *length = (cps - output);
    }
    buffer->offset = (size_t)(p - buffer->content);
    /* skip \" or \' */
    buffer->offset++;

    if (!_cc_buf_jump_comment(buffer)) {
        goto JSON_FAIL;
    }
    return output;

JSON_FAIL:
    if (output) {
        buffer->offset = (size_t)(start - buffer->content);
        _cc_free(output);
    }
    return nullptr;
}

_CC_API_PRIVATE(bool_t) _json_parser_number(_cc_sbuf_t *const buffer, _cc_json_t *const item) {
    _cc_number_t num;
    const tchar_t *start = _cc_sbuf_offset(buffer);
    const tchar_t *s = _cc_to_number(start, &num);

    if (_cc_unlikely(s == nullptr)) {
        return false;
    }

    if (num.vt == _CC_NUMBER_INT_) {
        item->type = _CC_JSON_INT_;
        item->element.uni_int = (int64_t)num.v.uni_int;
    } else {
        item->type = _CC_JSON_FLOAT_;
        item->element.uni_float = (float64_t)num.v.uni_float;
    }

    buffer->offset += (size_t)(s - start);

    return _cc_buf_jump_comment(buffer);
}

_CC_API_PRIVATE(bool_t) _json_parser_key_and_value(_cc_sbuf_t *const buffer, _cc_json_t *item) {
    /*parse the name of the key*/
    item->name = _json_parser_string(buffer,nullptr);
    if (_cc_unlikely(!item->name)) {
        return false;
    }

    if (_cc_unlikely(!_cc_sbuf_access(buffer) || _cc_sbuf_offset_unequal(buffer, _T(':')))) {
        _cc_free(item->name);
        return false;
    }
    /* skip : */
    buffer->offset++;
    _cc_buf_jump_comment(buffer);

    /*parse the value*/
    if (_cc_unlikely(!_json_read(buffer, item))) {
        _cc_free(item->name);
        /*failed to parse value*/
        return false;
    }

    return true;
}

static bool_t _json_parser_object(_cc_sbuf_t *const buffer, _cc_json_t *item) {
    _cc_json_t *curr_item = nullptr;
    _CC_RB_INIT_ROOT(&item->element.uni_object);

    if (_cc_unlikely(buffer->depth >= _JSON_NESTING_LIMIT_)) {
        /* to deeply nested */
        return false;
    }
    buffer->depth++;
    /*skip { */
    buffer->offset++;

    /* check if we skipped to the end of the buffer */
    if (!_cc_buf_jump_comment(buffer)) {
        buffer->offset--;
        goto JSON_FAIL;
    }

    if (_cc_unlikely(_cc_sbuf_offset_equal(buffer, _JSON_OBJECT_END_))) {
        goto JSON_SUCCESS;
    }

    /* step back to character in front of the first element */
    buffer->offset--;
    do {
        /*parse next value */
        buffer->offset++;
        if (_cc_buf_jump_comment(buffer) && _cc_sbuf_offset_equal(buffer,_JSON_OBJECT_END_))
            break;

        curr_item = (_cc_json_t *)_cc_malloc(sizeof(_cc_json_t));
        bzero(curr_item, sizeof(_cc_json_t));
        curr_item->type = _CC_JSON_NULL_;
        curr_item->size = 0;
        curr_item->length = 0;

        if (_cc_unlikely(!_json_parser_key_and_value(buffer, curr_item))) {
            _cc_destroy_json(&curr_item);
            goto JSON_FAIL;
        }

        if (_cc_unlikely(!_cc_rbtree_push(&item->element.uni_object, &curr_item->lnk, _json_push_object))) {
            _cc_destroy_json(&curr_item);
            goto JSON_FAIL;
        }
        item->length++;
    } while (_cc_sbuf_access(buffer) && _cc_sbuf_offset_equal(buffer,_JSON_NEXT_TOKEN_));

    /*expected end of object*/
    if (!_cc_sbuf_access(buffer) || _cc_sbuf_offset_unequal(buffer,_JSON_OBJECT_END_)) {
        goto JSON_FAIL;
    }

JSON_SUCCESS:
    item->type = _CC_JSON_OBJECT_;
    buffer->offset++;
    buffer->depth--;

    _cc_buf_jump_comment(buffer);

    return true;

JSON_FAIL:
    item->type = _CC_JSON_OBJECT_;
    _destroy_json_object(item);
    return false;
}

static bool_t _json_parser_array(_cc_sbuf_t *const buffer, _cc_json_t *item) {
    _cc_json_t *curr_item = nullptr;
    _json_array_alloc(item,32);

    if (_cc_unlikely(buffer->depth >= _JSON_NESTING_LIMIT_)) {
        /* to deeply nested */
        return false;
    }
    buffer->depth++;

    /*skip [ */
    buffer->offset++;

    /* check if we skipped to the end of the buffer */
    if (_cc_unlikely(!_cc_buf_jump_comment(buffer))) {
        buffer->offset--;
        goto JSON_FAIL;
    }

    if (_cc_unlikely(_cc_sbuf_offset_equal(buffer,_JSON_ARRAY_END_))) {
        goto JSON_SUCCESS;
    }

    /* step back to character in front of the first element */
    buffer->offset--;
    /*loop through the comma separated array elements*/
    do {
        /*parse next value */
        buffer->offset++;
        if (_cc_buf_jump_comment(buffer) && _cc_sbuf_offset_equal(buffer,_JSON_ARRAY_END_)) {
            break;
        }

        curr_item = (_cc_json_t *)_cc_malloc(sizeof(_cc_json_t));
        bzero(curr_item, sizeof(_cc_json_t));
        curr_item->type = _CC_JSON_NULL_;
        curr_item->size = 0;
        curr_item->length = 0;

        if (!_json_read(buffer, curr_item)) {
            _cc_destroy_json(&curr_item);
            goto JSON_FAIL;
        }

        if (_cc_unlikely(_json_array_push(item, curr_item) == -1)) {
            _cc_destroy_json(&curr_item);
            goto JSON_FAIL;
        }
    } while (_cc_sbuf_access(buffer) && _cc_sbuf_offset_equal(buffer,_JSON_NEXT_TOKEN_));

    /*expected end of object*/
    if (!_cc_sbuf_access(buffer) || _cc_sbuf_offset_unequal(buffer,_JSON_ARRAY_END_)) {
        goto JSON_FAIL;
    }

JSON_SUCCESS:
    item->type = _CC_JSON_ARRAY_;
    buffer->offset++;
    buffer->depth--;

    _cc_buf_jump_comment(buffer);

    return true;

JSON_FAIL:
    _destroy_json_array(item);

    return false;
}

_CC_API_PRIVATE(bool_t) _json_read(_cc_sbuf_t *const buffer, _cc_json_t *item) {
    if (buffer == nullptr || buffer->content == nullptr) {
        return false;
    }

    if (_cc_sbuf_access(buffer)) {
        register const tchar_t *p = _cc_sbuf_offset(buffer);
        if (*p == _T('-') || _CC_ISDIGIT(*p)) {
            return _json_parser_number(buffer, item);
        }
        if (*p == _JSON_OBJECT_START_) {
            return _json_parser_object(buffer, item);
        }
        if (*p == _JSON_ARRAY_START_) {
            return _json_parser_array(buffer, item);
        }

        if (_cc_sbuf_can_read(buffer, 4)) {
            if (_tcsncmp(p, _T("true"), 4) == 0) {
                item->type = _CC_JSON_BOOLEAN_;
                item->element.uni_boolean = true;
                buffer->offset += 4;
                return _cc_buf_jump_comment(buffer);
            }

            if (_tcsncmp(p, _T("null"), 4) == 0) {
                item->type = _CC_JSON_NULL_;
                item->element.uni_string = nullptr;
                buffer->offset += 4;
                return _cc_buf_jump_comment(buffer);
            }
        }

        if (_cc_sbuf_can_read(buffer, 5)) {
            if (_tcsncmp(p, _T("false"), 5) == 0) {
                item->type = _CC_JSON_BOOLEAN_;
                item->element.uni_boolean = false;
                buffer->offset += 5;
                return _cc_buf_jump_comment(buffer);
                ;
            }
        }

        item->element.uni_string = _json_parser_string(buffer,&item->length);
        if (item->element.uni_string) {
            item->type = _CC_JSON_STRING_;
            return true;
        }
    }

    return false;
}

_CC_API_PUBLIC(_cc_json_t*) _cc_josn_parser(_cc_sbuf_t *const buffer) {
    _cc_json_t *item = nullptr;
    _cc_json_error_t local_error;

    local_error.content = nullptr;
    local_error.position = 0;

    item = (_cc_json_t *)_cc_malloc(sizeof(_cc_json_t));
    bzero(item, sizeof(_cc_json_t));
    item->type = _CC_JSON_NULL_;
    item->size = 0;
    item->length = 0;
    if (_cc_buf_jump_comment(buffer) && _json_read(buffer, item)) {
        return item;
    }

    local_error.content = buffer->content;
    if (buffer->offset < buffer->length) {
        local_error.position = buffer->offset - 1;
    } else if (buffer->length > 0) {
        local_error.position = buffer->length - 1;
    }

    /*reset error position*/
    _cc_global_json_error = local_error;

    _cc_destroy_json(&item);
    return nullptr;
}

_CC_API_PUBLIC(_cc_json_t*) _cc_json_from_file(const tchar_t *file_name) {
    _cc_sbuf_t buffer;
    _cc_json_t *item = nullptr;

    byte_t *content = nullptr;
    size_t offset = 0;

    _cc_buf_t *buf = _cc_buf_from_file(file_name);

    if (_cc_unlikely(buf == nullptr)) {
        return nullptr;
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
    }

#ifdef _CC_UNICODE_
    _cc_buf_utf8_to_utf16(buf, offset);
    buffer.content = (tchar_t *)buf->bytes;
    buffer.length = _cc_buf_length(buf) / sizeof(tchar_t);
#else
    buffer.content = (tchar_t *)(content + offset);
    buffer.length = (_cc_buf_length(buf) - offset) / sizeof(tchar_t);
#endif

    buffer.offset = 0;
    buffer.line = 1;
    buffer.depth = 0;

    item = _cc_josn_parser(&buffer);

    _cc_destroy_buf(&buf);

    return item;
}

_CC_API_PUBLIC(_cc_json_t*) _cc_json_parse(const tchar_t *src, size_t length) {
    _cc_sbuf_t buffer;
    if (length == -1) {
        length = _tcslen(src);
    }
    buffer.content = src;
    buffer.length = length;
    buffer.offset = 0;
    buffer.line = 1;
    buffer.depth = 0;

    return _cc_josn_parser(&buffer);
}
