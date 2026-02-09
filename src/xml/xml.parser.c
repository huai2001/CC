#include "xml.c.h"
#include <libcc/UTF.h>

typedef struct {
    const tchar_t *name;
    byte_t length;
    tchar_t value;
} _xml_entity_t;

static const _xml_entity_t _xml_entities[] = {
    {_T("quot;"), 5, '\"'},
    {_T("apos;"), 5, '\''},
    {_T("amp;"),  4, '&'},
    {_T("lt;"),   3, '<'},
    {_T("gt;"),   3, '>'},
};

_CC_API_PRIVATE(_cc_sds_t) _unescape(const tchar_t *ptr, const tchar_t *endptr) {
    _cc_sds_t output;
    tchar_t *dst_ptr;
    tchar_t *dst_endptr;

    size_t alloc_length = (size_t)(endptr - ptr) + 1;

    if (alloc_length == 1) {
        return _cc_sds_alloc(NULL, 1);
    }
    dst_ptr = output = _cc_sds_alloc(NULL, alloc_length);
    dst_endptr = (output + alloc_length);
    
    while (ptr < endptr && dst_ptr < dst_endptr) {
        if (*ptr == _T('&')) {
            const tchar_t *entity = ptr + 1;
            size_t remaining = (size_t)(endptr - entity),i;
            for (i = 0; i < _cc_countof(_xml_entities); i++) {
                const _xml_entity_t *ent = &_xml_entities[i];
                if (remaining >= ent->length && entity[0] == ent->name[0] && _tcsnicmp(entity, ent->name, ent->length) == 0) {
                    *dst_ptr++ = ent->value;
                    ptr += ent->length + 1;
                    break;
                }
            }
        } else {
            *dst_ptr++ = *ptr++;
        }
    }

    *dst_ptr = '\0';
    _cc_sds_set_length(output, (size_t)(dst_ptr - output));
    return output;
}

_CC_API_PRIVATE(bool_t) _XML_is_name_start_char(int ch) {
    if (ch >= 128) {
        // This is a heuristic guess in attempt to not implement Unicode-aware
        // isalpha()
        return true;
    }

    if (_CC_ISALPHA(ch)) {
        return true;
    }

    return ch == ':' || ch == '_';
}

_CC_API_PRIVATE(bool_t) _XML_is_name_char(int ch) {
    return _XML_is_name_start_char(ch) || _CC_ISDIGIT(ch) || ch == '.' || ch == '-';
}

_CC_API_PRIVATE(_cc_sds_t) _XML_parser_name(_cc_sbuf_t *const buffer) {
    _cc_sds_t output = NULL;
    const tchar_t *ptr = _cc_sbuf_offset(buffer), *endptr = NULL;
    const tchar_t *p = ptr;

    while (*p && ((size_t)(p - buffer->content) < buffer->length)) {
        if (*p == _XML_ELEMENT_END_ || *p == '=' || _cc_isspace(*p)) {
            endptr = p;
            break;
        }
        if (*p == _XML_ELEMENT_SLASH_ && *(p + 1) == _XML_ELEMENT_END_) {
            endptr = p;
            break;
        }
        if (!_XML_is_name_char(*p)) {
            break;
        }
        p++;
    }

    if (!endptr) {
        return NULL;
    }

    output = _cc_sds_alloc(ptr, (size_t)(endptr - ptr));
    buffer->offset = (size_t)(p - buffer->content);

    return output;
}

_CC_API_PRIVATE(_cc_sds_t) _XML_parser_doctype(_cc_sbuf_t *const buffer) {
    const tchar_t *p = _cc_sbuf_offset(buffer);
    const tchar_t *ptr = p;

    while (*p && ((size_t)(p - buffer->content) < buffer->length)) {
        if (*p == _XML_ELEMENT_END_) {
            break;
        }
        p++;
    }

    buffer->offset = (size_t)(p - buffer->content) + 1;
    return _cc_sds_alloc(ptr, (size_t)(p - ptr));
}

_CC_API_PRIVATE(bool_t) _XML_parser_comments(_cc_sbuf_t *const buffer, _cc_xml_t *item) {
    const tchar_t *p = _cc_sbuf_offset(buffer);
    const tchar_t *ptr = p;
    const tchar_t *endptr = buffer->content + buffer->length;
    /* calculate approximate size of the output (overestimate) */
    while (p < endptr) {
        /* <!-- comments --> */
        if (*p == '-' && *(p + 1) == '-' && *(p + 2) == '>') {
            break;
        }
        if (*p == _T(_CC_LF_)) {
            buffer->line++;
        }
        p++;
    }

    if (p >= endptr) {
        return false;
    }

    item->element.uni_comment = _cc_sds_alloc(ptr, (size_t)(p - ptr));
    /* +3 skip --> */
    buffer->offset = (size_t)(p - buffer->content) + 3;
    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _XML_text_parser(_cc_sbuf_t *const buffer, _cc_xml_context_t *context) {
    const tchar_t *p = _cc_sbuf_offset(buffer);
    _cc_buf_t buf;

    size_t alloc_length = 0;
    byte_t cdata = context->cdata;

    const tchar_t *ptr = p;
    const tchar_t *endptr = NULL;

    _cc_alloc_buf(&buf, 1024);
    endptr = buffer->content + buffer->length;

    while (p < endptr) {
        if (cdata == 0 && *p == _XML_ELEMENT_START_) {
            if (ptr != p) {
                _cc_buf_append(&buf, (byte_t *)ptr, sizeof(tchar_t) * (p - ptr));
            }

            if (*(p + 1) == '!' && _tcsncmp(_T("[CDATA["), p + 2, 7) == 0) {
                p += 9;
                ptr = p;
                cdata = true;
                context->cdata = true;
                continue;
            } else {
                break;
            }
        }

        /* <![CDATA[ Unparsed Character Data]]> */
        if (cdata && *p == ']' && *(p + 1) == ']' && *(p + 2) == '>') {
            if (ptr != (p - 1)) {
                _cc_buf_append(&buf, (byte_t *)ptr, sizeof(tchar_t) * (p - ptr));
            }
            p += 3;
            ptr = p;
            cdata = 0;
            continue;
        }
        p++;
    }

    buffer->offset = (size_t)(p - buffer->content);

    if (buf.length <= 0) {
        context->text = _cc_sds_alloc(NULL, 1);
    } else {
        ptr = _cc_buf_stringify(&buf, &alloc_length);
        context->text = _unescape(ptr, (const tchar_t *)ptr + alloc_length);
    }
    _cc_free_buf(&buf);
    return context->text != NULL;
}

_CC_API_PRIVATE(int32_t) _XML_is_attr_value_end_tag(const tchar_t *p, const tchar_t quotes) {
    if (*p == _XML_ELEMENT_END_) {
        return 1;
    }

    if ((*p == _XML_ELEMENT_SLASH_ && *(p + 1) == _XML_ELEMENT_END_)) {
        return 2;
    }

    if (quotes == 0) {
        int32_t i = 0;
        while (_cc_isspace(*(p + i))) {
            i++;
        }
        return i;
    }

    return quotes == *p ? 1 : 0;
}

_CC_API_PRIVATE(_cc_sds_t) _XML_parser_attr_value(_cc_sbuf_t *const buffer) {
    const tchar_t *p = _cc_sbuf_offset(buffer);
    const tchar_t *ptr = NULL;
    const tchar_t *endptr = NULL;
    _cc_sds_t output = NULL;
    tchar_t quotes = *p;
    int32_t endflag = 0;

    endptr = buffer->content + buffer->length;
    if (_cc_likely(quotes == _T('"') || quotes == _T('\''))) {
        ptr = ++p;
    } else {
        quotes = 0;
        while (p < endptr && _CC_ISSPACE(*p)) {
            p++;
        }
        ptr = p;
    }

    /* calculate approximate size of the output (overestimate) */
    while (p < endptr && (endflag = _XML_is_attr_value_end_tag(p, quotes)) == 0) {
        p++;
    }

    if (p >= endptr || endflag == 0) {
        return NULL;
    }

    endptr = p;
    if (quotes) {
        while (ptr < endptr && _CC_ISSPACE(*(endptr - 1))) {
            endptr--;
        }
    }
    output = _unescape(ptr, endptr);
    if (!output) {
        return NULL;
    }
    buffer->offset = (size_t)(p - buffer->content) + endflag;
    return output;
}

_CC_API_PRIVATE(int) _XML_attr_read(_cc_rbtree_t *ctx, _cc_sbuf_t *const buffer) {
    const tchar_t *tmp;

    do {
        _cc_sds_t name = NULL;
        _cc_sds_t value = NULL;
        if (!_XML_jump_whitespace(buffer)) {
            return false;
        }

        tmp = _cc_sbuf_offset(buffer);
        /*expected end of XML*/
        switch (*tmp) {
        case _XML_ELEMENT_END_:
            buffer->offset++;
            return 1;
            break;
        case _XML_ELEMENT_SLASH_:
            if (*(tmp + 1) == _XML_ELEMENT_END_) {
                buffer->offset += 2;
                return 2;
            }
            break;
        case '?':
            if (*(tmp + 1) == _XML_ELEMENT_END_) {
                buffer->offset += 2;
                return 3;
            }
            break;
        }
        /*
        ** ! parse the name of the key
        */
        name = _XML_parser_name(buffer);
        if (name == NULL) {
            break;
        }

        if (!_XML_jump_whitespace(buffer)) {
            _cc_sds_free(name);
            break;
        }

        if (_cc_sbuf_access(buffer) && _cc_sbuf_offset_equal(buffer, '=')) {
            /*
            ** ! skip =
            */
            buffer->offset++;
            if (!_XML_jump_whitespace(buffer)) {
                _cc_sds_free(name);
                break;
            }
            /*parse the value*/
            value = _XML_parser_attr_value(buffer);
            if (value == NULL) {
                _cc_sds_free(name);
                break;
            }
        }
        _XML_attr_push(ctx, name, value);
    } while (_cc_sbuf_access(buffer));

    return 0;
}

static bool_t _XML_child_read(_cc_xml_t *ctx, _cc_sbuf_t *const buffer, int32_t depth) {
    _cc_xml_t *item;
    do {
        int tailed = 0;
        const tchar_t *p;

        if (!_XML_jump_whitespace(buffer)) {
            return true;
        }

        p = _cc_sbuf_offset(buffer);

        if (*p == _XML_ELEMENT_START_ && *(p + 1) == '/') {
            return true;
        }

        item = (_cc_xml_t *)_cc_malloc(sizeof(_cc_xml_t));
        _XML_NODE_INIT(item, _CC_XML_NULL_);
        _cc_list_push(&ctx->element.uni_child, &item->lnk);

        if (*p == _XML_ELEMENT_START_) {
            /* */
            if (*(p + 1) == '!') {
                /*
                **! read comments
                */
                if (*(p + 2) == '-' && *(p + 3) == '-') {
                    buffer->offset += 4;
                    if (!_XML_parser_comments(buffer, item)) {
                        return false;
                    }
                    item->type = _CC_XML_COMMENT_;
                } else if (_tcsncmp(_T("[CDATA["), p + 2, 7) == 0) {
                    buffer->offset += 9;
                    item->element.uni_context.cdata = 1;
                    if (!_XML_text_parser(buffer, &item->element.uni_context)) {
                        return false;
                    }
                    item->type = _CC_XML_CONTEXT_;
                } else if (_tcsncmp(_T("DOCTYPE"), p + 2, 7) == 0) {
                    buffer->offset += 9;
                    if (!_XML_jump_whitespace(buffer)) {
                        return false;
                    }
                    item->element.uni_doctype = _XML_parser_doctype(buffer);
                    item->type = _CC_XML_DOCTYPE_;
                } else {
                    return false;
                }
            } else {
                /* skip < */
                buffer->offset++;
                item->name = _XML_parser_name(buffer);
                if (item->name == NULL) {
                    return false;
                }

                tailed = _XML_attr_read(&item->attr, buffer);
                if (tailed == 0) {
                    return false;
                }

                if (tailed == 2 || tailed == 3) {
                    continue;
                }

                item->type = _CC_XML_CHILD_;
                _cc_list_cleanup(&item->element.uni_child);

                if (!_XML_child_read(item, buffer, depth + 1)) {
                    return false;
                }

                if (_cc_list_empty(&item->element.uni_child)) {
                    item->type = _CC_XML_NULL_;
                }

                if (!_XML_jump_whitespace(buffer)) {
                    return false;
                }

                p = _cc_sbuf_offset(buffer);
                if (*p == _XML_ELEMENT_START_ && *(p + 1) == _XML_ELEMENT_SLASH_) {
                    size_t tag_lengtn = _cc_sds_length(item->name);
                    /* skip </ */
                    if (_tcsncmp(p + 2, item->name, tag_lengtn) != 0) {
                        return false;
                    }
                    /* skip </tag-name */
                    buffer->offset += tag_lengtn + 2;
                    if (_cc_sbuf_offset_unequal(buffer, _XML_ELEMENT_END_)) {
                        return false;
                    }
                    /* skip > */
                    buffer->offset++;
                }
            }
        } else {
            item->type = _CC_XML_CONTEXT_;
            item->element.uni_context.cdata = 0;
            if (!_XML_text_parser(buffer, &item->element.uni_context)) {
                return false;
            }
        }
    } while (_cc_sbuf_access(buffer));

    return true;
}

_CC_API_PRIVATE(bool_t) _XML_read(_cc_xml_t *ctx, _cc_sbuf_t *const buffer) {
    const tchar_t *p = _cc_sbuf_offset(buffer);
    if (*p == _XML_ELEMENT_START_ && *(p + 1) == '?') {
        /* skip <? */
        buffer->offset += 2;

        ctx->name = _XML_parser_name(buffer);
        if (_cc_unlikely(ctx->name == NULL)) {
            return false;
        }
        if (_XML_attr_read(&ctx->attr, buffer) == 0) {
            return false;
        }
    }

    if (!_XML_child_read(ctx, buffer, 0)) {
        return false;
    }
    return true;
}

_CC_API_PUBLIC(_cc_xml_t*) _cc_xml_parser(_cc_sbuf_t *const buffer) {
    _cc_xml_t *item = NULL;
    _cc_syntax_error_t local_error;

    local_error.content = NULL;
    local_error.position = 0;

    item = (_cc_xml_t *)_cc_malloc(sizeof(_cc_xml_t));
    _XML_NODE_INIT(item, _CC_XML_CHILD_);

    if (_XML_read(item, buffer)) {
        return item;
    }

    local_error.content = buffer->content;
    if (buffer->offset < buffer->length) {
        local_error.position = buffer->offset;
    } else if (buffer->length > 0) {
        local_error.position = buffer->length - 1;
    }

    /*reset error position*/
    _cc_syntax_error(&local_error);

    _cc_free_xml(item);
    return NULL;
}

_CC_API_PUBLIC(_cc_xml_t*) _cc_xml_from_file(const tchar_t *file_name) {
    _cc_sbuf_t buffer;
    _cc_xml_t *item = NULL;
    _cc_buf_t buf;

    if (!_cc_buf_from_file(&buf, file_name)) {
        return NULL;
    }

    buffer.content = (tchar_t*)buf.bytes;
    buffer.length = buf.length / sizeof(tchar_t);
    buffer.offset = 0;
    buffer.line = 1;
    buffer.depth = 0;

    item = _cc_xml_parser(&buffer);

    _cc_free_buf(&buf);

    return item;
}

_CC_API_PUBLIC(_cc_xml_t*) _cc_xml_parse(const tchar_t *src, size_t length) {
    _cc_sbuf_t buffer;
    if (length == -1) {
        length = _tcslen(src);
    }
    buffer.content = src;
    buffer.length = length;
    buffer.offset = 0;
    buffer.line = 1;
    buffer.depth = 0;

    return _cc_xml_parser(&buffer);
}
