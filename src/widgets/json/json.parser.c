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
#include "json.c.h"

_CC_API_PRIVATE(bool_t) _json_read(_cc_sbuf_t *const buffer, _cc_json_t *item);

_CC_API_PUBLIC(tchar_t*) _sbuf_parser_string(_cc_sbuf_t *const buffer, size_t *length) {
    const tchar_t *p = _cc_sbuf_offset(buffer);
    const tchar_t *start = nullptr;
    const tchar_t *endpos;
    size_t alloc_length = 0;
    size_t skipped_bytes = 0;
    tchar_t *output = nullptr;
    tchar_t quotes = *p;

    if (_cc_likely(quotes == _T('"') || quotes == _T('\''))) {
        start = ++p;
    } else {
        return nullptr;
    }

    endpos = buffer->content + buffer->length;
    /* calculate approximate size of the output (overestimate) */
    while (p < endpos && (*p != quotes)) {
        if (*p == _T('\\')) {
            /* prevent buffer overflow when last input character is a backslash */
            if ((p + 1) >= endpos) {
                return nullptr;
            }
            skipped_bytes++;
            p++;
        }
        p++;
    }

    if (p >= endpos || *p != quotes) {
        return nullptr;
    }

    /* This is at most how much we need for the output */
    alloc_length = sizeof(tchar_t) * ((size_t)(p - start) - skipped_bytes + 1);
    output = (tchar_t *)_cc_malloc(alloc_length);
    endpos = _convert_text(output, alloc_length, start, p);
    if (endpos) {
        if (length) {
            *length = (endpos - output);
        }
        /* +1 skip \" or \' */
        buffer->offset = (size_t)(p - buffer->content) + 1;

        return output;
    }

    _cc_free(output);
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

_CC_API_PRIVATE(bool_t) _json_parser_key_and_value(_cc_sbuf_t *const buffer, _cc_json_t *root) {
    _cc_json_t *item;
    /*parse the name of the key*/
    tchar_t *name = _sbuf_parser_string(buffer,nullptr);
    if (_cc_unlikely(!name)) {
        return false;
    }

    if (!_cc_buf_jump_comment(buffer)) {
        _cc_free(name);
        return false;
    }
    
    if (_cc_unlikely(!_cc_sbuf_access(buffer) || _cc_sbuf_offset_unequal(buffer, _T(':')))) {
        _cc_free(name);
        return false;
    }
    /* skip : */
    buffer->offset++;
    _cc_buf_jump_comment(buffer);
    item = _json_object_push(root, name);
    _cc_free(name);

    /*parse the value*/
    if (_cc_unlikely(!_json_read(buffer, item))) {
        /*failed to parse value*/
        return false;
    }

    return true;
}

static bool_t _json_parser_object(_cc_sbuf_t *const buffer, _cc_json_t *item) {
    _CC_RB_INIT_ROOT(&item->element.uni_object);
    item->type = _CC_JSON_OBJECT_;

    if (buffer->depth >= _JSON_NESTING_LIMIT_) {
        /* to deeply nested */
        return false;
    }
    buffer->depth++;
    /*skip { */
    buffer->offset++;

    if (!_cc_buf_jump_comment(buffer)) {
        buffer->offset--;
        goto JSON_FAIL;
    }

    if (_cc_sbuf_offset_equal(buffer, _JSON_OBJECT_END_)) {
        /* empty object */
        goto JSON_SUCCESS;
    }

    /* step back to character in front of the first element */
    buffer->offset--;
    /* loop through the comma separated array elements */
    do {
        /*parse next value */
        buffer->offset++;
        if (_cc_buf_jump_comment(buffer) && _cc_sbuf_offset_equal(buffer,_JSON_OBJECT_END_))
            break;

        if (!_json_parser_key_and_value(buffer, item)) {
            goto JSON_FAIL;
        }

        item->length++;
    } while (_cc_sbuf_access(buffer) && _cc_sbuf_offset_equal(buffer,_JSON_NEXT_TOKEN_));

    /*expected end of object*/
    if (!_cc_sbuf_access(buffer) || _cc_sbuf_offset_unequal(buffer,_JSON_OBJECT_END_)) {
        goto JSON_FAIL;
    }

JSON_SUCCESS:
    /* skip _JSON_OBJECT_END_ */
    buffer->offset++;
    buffer->depth--;
    /**/
    _cc_buf_jump_comment(buffer);

    return true;

JSON_FAIL:
    _destroy_json_object(item);
    return false;
}

static bool_t _json_parser_array(_cc_sbuf_t *const buffer, _cc_json_t *root) {
    _cc_json_t *curr_item = nullptr;
    _json_array_alloc(root,32);

    if (buffer->depth >= _JSON_NESTING_LIMIT_) {
        /* to deeply nested */
        return false;
    }
    buffer->depth++;

    /*skip [ */
    buffer->offset++;

    if (!_cc_buf_jump_comment(buffer)) {
        buffer->offset--;
        goto JSON_FAIL;
    }

    if (_cc_sbuf_offset_equal(buffer,_JSON_ARRAY_END_)) {
        /* not an array */
        goto JSON_SUCCESS;
    }

    /* step back to character in front of the first element */
    buffer->offset--;
    /*loop through the comma separated array elements*/
    do {
        /* parse next value */
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

        if (_cc_unlikely(_json_array_push(root, curr_item) == -1)) {
            _cc_destroy_json(&curr_item);
            goto JSON_FAIL;
        }
    } while (_cc_sbuf_access(buffer) && _cc_sbuf_offset_equal(buffer,_JSON_NEXT_TOKEN_));

    /*expected end of array*/
    if (!_cc_sbuf_access(buffer) || _cc_sbuf_offset_unequal(buffer,_JSON_ARRAY_END_)) {
        /* invalid array */
        goto JSON_FAIL;
    }

JSON_SUCCESS:
    root->type = _CC_JSON_ARRAY_;
    /* skip _JSON_ARRAY_END_ */
    buffer->offset++;
    buffer->depth--;

    /**/
    _cc_buf_jump_comment(buffer);

    return true;

JSON_FAIL:
    _destroy_json_array(root);

    return false;
}

_CC_API_PRIVATE(bool_t) _json_read(_cc_sbuf_t *const buffer, _cc_json_t *item) {
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
        }
    }

    item->element.uni_string = _sbuf_parser_string(buffer,&item->length);
    if (item->element.uni_string) {
        item->type = _CC_JSON_STRING_;
        return _cc_buf_jump_comment(buffer);
    }

    return false;
}

_CC_API_PUBLIC(_cc_json_t*) _cc_josn_parser(_cc_sbuf_t *const buffer) {
    _cc_json_t *item = nullptr;
    _cc_syntax_error_t local_error;

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
    _cc_syntax_error(&local_error);

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

_CC_API_PUBLIC(const tchar_t*) _cc_json_error(void) {
    return _cc_get_syntax_error();
}