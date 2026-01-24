#include "json.c.h"

_CC_API_PRIVATE(bool_t) _json_read(_cc_sbuf_t *const buffer, _cc_json_t *item);

_CC_API_PUBLIC(_cc_sds_t) _sbuf_parser_string(_cc_sbuf_t *const buffer) {
    const tchar_t *p = _cc_sbuf_offset(buffer);
    const tchar_t *start = NULL;
    const tchar_t *endpos;
    size_t alloc_length = 0;
    size_t skipped_bytes = 0;
    _cc_sds_t output = NULL;
    tchar_t quotes = *p;

    if (_cc_likely(quotes == _T('"') || quotes == _T('\''))) {
        start = ++p;
    } else {
        return NULL;
    }

    endpos = buffer->content + buffer->length;
    /* calculate approximate size of the output (overestimate) */
    while (p < endpos && (*p != quotes)) {
        if (*p == _T('\\')) {
            /* prevent buffer overflow when last input character is a backslash */
            if ((p + 1) >= endpos) {
                return NULL;
            }
            skipped_bytes++;
            p++;
        }
        p++;
    }

    if (p >= endpos || *p != quotes) {
        return NULL;
    }

    /* This is at most how much we need for the output */
    alloc_length = sizeof(tchar_t) * ((size_t)(p - start) - skipped_bytes + 1);
    output = _cc_sds_alloc(NULL, alloc_length);
    endpos = _convert_text(output, start, p);
    if (endpos) {
        /* +1 skip \" or \' */
        buffer->offset = (size_t)(p - buffer->content) + 1;
        return output;
    }

    _cc_sds_free(output);
    return NULL;
}

_CC_API_PRIVATE(bool_t) _json_parser_number(_cc_sbuf_t *const buffer, _cc_json_t *const item) {
    _cc_number_t num;
    const tchar_t *start = _cc_sbuf_offset(buffer);
    const tchar_t *s = _cc_to_number(start, &num);

    if (_cc_unlikely(s == NULL)) {
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
    _cc_json_t *curr_item;
    size_t offset = buffer->offset;
    /*parse the name of the key*/
    _cc_sds_t name = _sbuf_parser_string(buffer);
    if (_cc_unlikely(!name)) {
        buffer->offset = offset;
        return false;
    }

    if (!_cc_buf_jump_comment(buffer)) {
        buffer->offset = offset;
        _cc_sds_free(name);
        return false;
    }
    
    if (_cc_unlikely(!_cc_sbuf_access(buffer) || _cc_sbuf_offset_unequal(buffer, _JSON_OBJECT_TOKEN_))) {
        _cc_sds_free(name);
        buffer->offset = offset;
        return false;
    }
    /* skip : */
    buffer->offset++;

    if (!_cc_buf_jump_comment(buffer)) {
        buffer->offset = offset;
        _cc_sds_free(name);
        return false;
    }

    curr_item = (_cc_json_t *)_cc_malloc(sizeof(_cc_json_t));
    bzero(curr_item, sizeof(_cc_json_t));
    curr_item->type = _CC_JSON_NULL_;
    curr_item->name = name;

    /*parse the value*/
    if (!_json_read(buffer, curr_item)) {
        buffer->offset = offset;
        /*failed to parse value*/
        _json_free_node(curr_item);
        return false;
    }

    return _json_object_push(root, curr_item, true);
}

static bool_t _json_parser_object(_cc_sbuf_t *const buffer, _cc_json_t *item) {
    _cc_rbtree_cleanup(&item->element.uni_object);
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
        return false;
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
        if (_cc_buf_jump_comment(buffer) && _cc_sbuf_offset_equal(buffer,_JSON_OBJECT_END_)) {
            break;
        }

        if (!_json_parser_key_and_value(buffer, item)) {
            return false;
        }
    } while (_cc_sbuf_access(buffer) && _cc_sbuf_offset_equal(buffer,_JSON_NEXT_TOKEN_));

    /*expected end of object*/
    if (!_cc_sbuf_access(buffer) || _cc_sbuf_offset_unequal(buffer,_JSON_OBJECT_END_)) {
        return false;
    }

JSON_SUCCESS:
    /* skip _JSON_OBJECT_END_ */
    buffer->offset++;
    buffer->depth--;
    /**/
    _cc_buf_jump_comment(buffer);
    return true;
}

static bool_t _json_parser_array(_cc_sbuf_t *const buffer, _cc_json_t *root) {
    _cc_json_t *curr_item = NULL;
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
        return false;
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
        curr_item->name = NULL;
        curr_item->element.uni_object.rb_node = NULL;

        if (!_json_read(buffer, curr_item)) {
            _json_free_node(curr_item);
            return false;
        }
        _json_array_push(root, curr_item);
    } while (_cc_sbuf_access(buffer) && _cc_sbuf_offset_equal(buffer,_JSON_NEXT_TOKEN_));

    /*expected end of array*/
    if (!_cc_sbuf_access(buffer) || _cc_sbuf_offset_unequal(buffer,_JSON_ARRAY_END_)) {
        /* invalid array */
        return false;
    }

JSON_SUCCESS:
    /* skip _JSON_ARRAY_END_ */
    buffer->offset++;
    buffer->depth--;

    /**/
    _cc_buf_jump_comment(buffer);

    return true;
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
            item->element.uni_string = NULL;
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

    item->element.uni_string = _sbuf_parser_string(buffer);
    if (item->element.uni_string) {
        item->type = _CC_JSON_STRING_;
        return _cc_buf_jump_comment(buffer);
    }

    return false;
}

_CC_API_PUBLIC(_cc_json_t*) _cc_json_parser(_cc_sbuf_t *const buffer) {
    _cc_json_t *curr_item;
    _cc_syntax_error_t local_error;

    local_error.content = NULL;
    local_error.position = 0;

    if (!_cc_buf_jump_comment(buffer)) {
        return NULL;
    }

    curr_item = (_cc_json_t *)_cc_malloc(sizeof(_cc_json_t));
    bzero(curr_item, sizeof(_cc_json_t));
    curr_item->type = _CC_JSON_NULL_;

    if (_json_read(buffer, curr_item)) {
        return curr_item;
    }

    local_error.content = buffer->content;
    if (buffer->offset < buffer->length) {
        local_error.position = buffer->offset - 1;
    } else if (buffer->length > 0) {
        local_error.position = buffer->length - 1;
    }

    /*reset error position*/
    _cc_syntax_error(&local_error);

    _json_free_node(curr_item);
    return NULL;
}

_CC_API_PUBLIC(_cc_json_t*) _cc_json_from_file(const tchar_t *file_name) {
    _cc_sbuf_t buffer;
    _cc_json_t *item = NULL;
    _cc_buf_t buf;

    if (!_cc_buf_from_file(&buf, file_name)) {
        return NULL;
    }

    buffer.content = (tchar_t*)buf.bytes;
    buffer.length = buf.length / sizeof(tchar_t);
    buffer.offset = 0;
    buffer.line = 1;
    buffer.depth = 0;
    
    item = _cc_json_parser(&buffer);

    _cc_free_buf(&buf);

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

    return _cc_json_parser(&buffer);
}

_CC_API_PUBLIC(const tchar_t*) _cc_json_error(void) {
    return _cc_get_syntax_error();
}