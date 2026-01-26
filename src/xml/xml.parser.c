#include "xml.c.h"

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
    const tchar_t *start = _cc_sbuf_offset(buffer), *ended = NULL;
    const tchar_t *p = start;

    while (*p && ((size_t)(p - buffer->content) < buffer->length)) {
        if (*p == _XML_ELEMENT_END_ || *p == '=' || _cc_isspace(*p)) {
            ended = p;
            break;
        }
        if (*p == _XML_ELEMENT_SLASH_ && *(p + 1) == _XML_ELEMENT_END_) {
            ended = p;
            break;
        }
        if (!_XML_is_name_char(*p)) {
            break;
        }
        p++;
    }

    if (!ended) {
        return NULL;
    }

    output = _cc_sds_alloc(start, (size_t)(ended - start));
    buffer->offset = (size_t)(p - buffer->content);

    return output;
}

_CC_API_PRIVATE(_cc_sds_t) _XML_parser_doctype(_cc_sbuf_t *const buffer) {
    const tchar_t *p = _cc_sbuf_offset(buffer);
    const tchar_t *start = p;

    while (*p && ((size_t)(p - buffer->content) < buffer->length)) {
        if (*p == _XML_ELEMENT_END_) {
            break;
        }
        p++;
    }

    buffer->offset = (size_t)(p - buffer->content) + 1;
    return _cc_sds_alloc(start, (size_t)(p - start));
}

_CC_API_PRIVATE(bool_t) _XML_parser_comments(_cc_sbuf_t *const buffer, _cc_sds_t *output) {
    const tchar_t *p = _cc_sbuf_offset(buffer);
    const tchar_t *start = p;
    const tchar_t *endpos = NULL;
    size_t alloc_length = 0;
    size_t skipped_bytes = 0;

    endpos = buffer->content + buffer->length;
    /* calculate approximate size of the output (overestimate) */
    while (p < endpos) {
        /* <![CDATA[Unparsed Character Data]]> */
        if (*p == '-' && *(p + 1) == '-' && *(p + 2) == '>') {
            break;
        }

        if (*p == _T(_CC_LF_)) {
            buffer->line++;
        }

        if (*p == _T('\\')) {
            /* is escape sequence */
            if ((p + 1) >= endpos) {
                return false;
            }
            skipped_bytes++;
            p++;
        }
        p++;
    }

    if (p >= endpos) {
        return false;
    }

    /* This is at most how much we need for the output */
    alloc_length = sizeof(tchar_t) * ((size_t)(p - start) - skipped_bytes + 1);
    *output = _cc_sds_alloc(NULL, alloc_length);
    if (_convert_text(*output, start, p)) {
        buffer->offset = (size_t)(p - buffer->content) + 3;
        return true;
    }

    _cc_sds_free(*output);
    *output = NULL;

    return false;
}

/**/
_CC_API_PRIVATE(bool_t) _XML_text_parser(_cc_sbuf_t *const buffer, _cc_xml_context_t *context) {
    const tchar_t *p = _cc_sbuf_offset(buffer);
    _cc_buf_t buf;

    size_t alloc_length = 0;
    byte_t cdata = context->cdata;

    const tchar_t *start = p;
    const tchar_t *endpos = NULL;

    _cc_alloc_buf(&buf, 1024);
    endpos = buffer->content + buffer->length;

    while (p < endpos) {
        if (cdata == 0 && *p == _XML_ELEMENT_START_) {
            if (start != p) {
                _cc_buf_append(&buf, (byte_t *)start, sizeof(tchar_t) * (p - start));
            }

            if (*(p + 1) == '!' && _tcsncmp(_T("[CDATA["), p + 2, 7) == 0) {
                p += 9;
                start = p;
                cdata = true;
                context->cdata = true;
                continue;
            } else {
                break;
            }
        }

        /* <![CDATA[ Unparsed Character Data]]> */
        if (cdata && *p == ']' && *(p + 1) == ']' && *(p + 2) == '>') {
            if (start != (p - 1)) {
                _cc_buf_append(&buf, (byte_t *)start, sizeof(tchar_t) * (p - start - 1));
            }
            p += 3;
            start = p;
            cdata = 0;
            continue;
        }
        p++;
    }

    buffer->offset = (size_t)(p - buffer->content);

    if (buf.length <= 0) {
        _cc_free_buf(&buf);
        return false;
    }

    /* This is at most how much we need for the output */
    start = _cc_buf_stringify(&buf,&alloc_length);
    context->text = _cc_sds_alloc(NULL, alloc_length);
    if (_convert_text(context->text, (const tchar_t *)buf.bytes, (const tchar_t *)start + alloc_length)) {
        _cc_free_buf(&buf);
        return true;
    }

    _cc_sds_free(context->text);
    context->text = NULL;

    _cc_free_buf(&buf);
    return false;
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
    const tchar_t *start = NULL;
    const tchar_t *endpos = NULL;
    _cc_sds_t output = NULL;
    size_t alloc_length = 0;
    size_t skipped_bytes = 0;
    tchar_t quotes = *p;
    int32_t endflag = 0;

    endpos = buffer->content + buffer->length;
    if (_cc_likely(quotes == _T('"') || quotes == _T('\''))) {
        start = ++p;
    } else {
        quotes = 0;
        while (p < endpos && _CC_ISSPACE(*p)) {
            p++;
        }
        start = p;
    }

    /* calculate approximate size of the output (overestimate) */
    while (p < endpos && (endflag = _XML_is_attr_value_end_tag(p, quotes)) == 0) {
        if (*p == _T('\\')) {
            /* is escape sequence */
            if ((p + 1) >= endpos) {
                return NULL;
            }
            skipped_bytes++;
            p++;
        }
        p++;
    }

    if (p >= endpos || endflag == 0) {
        return NULL;
    }

    endpos = p;
    if (quotes) {
        while (start < endpos && _CC_ISSPACE(*(endpos - 1))) {
            endpos--;
        }
    }
    
    /* This is at most how much we need for the output */
    alloc_length = ((size_t)(endpos - start) - skipped_bytes + 1);
    output = _cc_sds_alloc(NULL, alloc_length);

    if (_convert_text(output, start, endpos)) {
        buffer->offset = (size_t)(p - buffer->content) + endflag;
        return output;
    }
    _cc_sds_free(output);
    return NULL;
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
                    if (!_XML_parser_comments(buffer, &item->element.uni_comment)) {
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
