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
#include "xml.c.h"

typedef struct {
    const tchar_t *content;
    size_t position;
} _cc_XML_error_t;

struct _XML_entity {
    const tchar_t *pattern;
    byte_t length;
    tchar_t value;
};

#define _XML_NUM_ENTITIES_ 5
static const struct _XML_entity XML_entities[_XML_NUM_ENTITIES_] = {{_T("quot"), 4, _T('\"')},
                                                                    {_T("apos"), 4, _T('\'')},
                                                                    {_T("amp"), 3, _T('&')},
                                                                    {_T("lt"), 2, _T('<')},
                                                                    {_T("gt"), 2, _T('>')}};

static _cc_XML_error_t _cc_global_XML_error = {NULL, 0};

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

_CC_API_PRIVATE(tchar_t*) _XML_parser_name(_cc_sbuf_tchar_t *const buffer) {
    tchar_t *output = NULL;
    const tchar_t *start = _cc_sbuf_offset(buffer), *ended = NULL;
    const tchar_t *p = start;

    while (*p && ((size_t)(p - buffer->content) < buffer->length)) {
        if (*p == _XML_ELEMENT_END_ || *p == '=' || _CC_ISSPACE(*p)) {
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

    output = (tchar_t *)_cc_tcsndup(start, (size_t)(ended - start));
    if (output == NULL) {
        return NULL;
    }

    buffer->offset = (size_t)(p - buffer->content);

    return output;
}

_CC_API_PRIVATE(bool_t) _XML_parser_doctype(_cc_sbuf_tchar_t *const buffer, tchar_t **output) {
    const tchar_t *p = _cc_sbuf_offset(buffer);
    const tchar_t *start = p;

    while (*p && ((size_t)(p - buffer->content) < buffer->length)) {
        if (*p == _XML_ELEMENT_END_) {
            break;
        }

        p++;
    }

    buffer->offset = (size_t)(p - buffer->content) + 1;

    *output = (tchar_t *)_cc_tcsndup(start, (size_t)(p - start));
    if (_cc_likely(*output != NULL)) {
        return true;
    }

    if (*output) {
        _cc_free(*output);
        *output = NULL;
    }
    return false;
}

_CC_API_PRIVATE(bool_t) _XML_convert_text(tchar_t *cps, size_t alloc_length, const tchar_t *src, const tchar_t *p) {
    size_t i = 0;
    int32_t convert_bytes = 0;
    while (src < p) {
        if (*src == _T('\\')) {
            switch (*(src + 1)) {
            case _T('b'):
                *cps++ = _T('\b');
                src += 2;
                break;
            case _T('f'):
                *cps++ = _T('\f');
                src += 2;
                break;
            case _T('n'):
                *cps++ = _T('\n');
                src += 2;
                break;
            case _T('r'):
                *cps++ = _T('\r');
                src += 2;
                break;
            case _T('t'):
                *cps++ = _T('\t');
                src += 2;
                break;
            case _T('\"'):
            case _T('\\'):
            case _T('/'): {
                *cps++ = *src;
                src += 2;
            } break;
                /* UTF-16 literal */
            case _T('u'): {
                /* failed to convert UTF16-literal to UTF-8 */
                src += 2;
                convert_bytes = _cc_convert_utf16_literal_to_utf8(&src, p, cps, alloc_length);
                if (_cc_unlikely(convert_bytes == 0)) {
                    return false;
                }
                cps += convert_bytes;
                break;
            }
            default:
                return false;
                break;
            }
        } else if (*src == _T('&')) {
            const struct _XML_entity *entity = NULL;
            for (i = 0; i < _XML_NUM_ENTITIES_; i++) {
                const struct _XML_entity *tmp = &XML_entities[i];
                if (*tmp->pattern == *(src + 1) && _tcsnicmp(tmp->pattern, src + 1, tmp->length) == 0) {
                    entity = tmp;
                    break;
                }
            }

            if (entity) {
                *cps++ = entity->value;
                /* +1 skip & */
                src += entity->length + 1;
                continue;
            }
        }
        *cps++ = *src++;
    }
    *cps = 0;
    return true;
}

_CC_API_PRIVATE(bool_t) _XML_parser_comments(_cc_sbuf_tchar_t *const buffer, tchar_t **output) {
    const tchar_t *p = _cc_sbuf_offset(buffer);
    const tchar_t *start = p;
    size_t alloc_length = 0;
    size_t skipped_bytes = 0;

    while (*p && ((size_t)(p - buffer->content) < buffer->length)) {
        /* <![CDATA[Unparsed Character Data]]> */
        if (*p == '-' && *(p + 1) == '-' && *(p + 2) == '>') {
            break;
        }
        if (*p == _T(_CC_LF_)) {
            buffer->line++;
        }

        if (*p == _T('\\') && ((size_t)((p + 1) - buffer->content) < buffer->length)) {
            skipped_bytes++;
            p++;
        }

        p++;
    }

    if (((size_t)(p - buffer->content) >= buffer->length)) {
        goto XML_FAIL;
    }

    /* This is at most how much we need for the output */
    alloc_length = sizeof(tchar_t) * ((size_t)(p - start) - skipped_bytes + 1);

    if (alloc_length <= 1) {
        buffer->offset = (size_t)(p - buffer->content) + 3;
        return true;
    }

    *output = (tchar_t *)_cc_malloc(alloc_length);
    if (_XML_convert_text(*output, alloc_length, start, p)) {
        buffer->offset = (size_t)(p - buffer->content) + 3;
        return true;
    }

XML_FAIL:
    if (*output) {
        _cc_free(*output);
        *output = NULL;
    }
    return false;
}

/**/
_CC_API_PRIVATE(bool_t) _XML_parser_text(_cc_sbuf_tchar_t *const buffer, _cc_xml_context_t *context) {
    const tchar_t *p = _cc_sbuf_offset(buffer);
    _cc_buf_t buf;

    size_t alloc_length = 0;
    byte_t cdata = context->cdata;

    _cc_buf_alloc(&buf, 1024);

    while (*p && ((size_t)(p - buffer->content) < buffer->length)) {
        if (cdata == 0 && *p == _XML_ELEMENT_START_) {
            if (*(p + 1) == '!' && _tcsncmp(_T("[CDATA["), p + 2, 7) == 0) {
                p += 9;
                cdata = true;
                context->cdata = true;
                continue;
            } else {
                break;
            }
        }

        /* <![CDATA[ Unparsed Character Data]]> */
        if (cdata && *p == ']' && *(p + 1) == ']' && *(p + 2) == '>') {
            p += 3;
            cdata = 0;
            continue;
        }
        _cc_buf_write(&buf, (byte_t *)p, sizeof(tchar_t));
        p++;
    }

    buffer->offset = (size_t)(p - buffer->content);

    if (buf.length <= 0) {
        _cc_buf_free(&buf);
        return false;
    }

    /* This is at most how much we need for the output */
    alloc_length = _cc_buf_length(&buf) + 1;
    context->text = (tchar_t *)_cc_malloc(alloc_length * sizeof(tchar_t));
    if (!_XML_convert_text(context->text, alloc_length, (const tchar_t *)buf.bytes, (const tchar_t *)buf.bytes + buf.length)) {
        _cc_free(context->text);
        _cc_buf_free(&buf);
        context->text = NULL;
        return false;
    }

    _cc_buf_free(&buf);
    return true;
}

_CC_API_PRIVATE(int32_t) _XML_is_attr_value_end_tag(const tchar_t *p, const tchar_t isquoted) {
    if (*p == _XML_ELEMENT_END_) {
        return 1;
    }

    if ((*p == _XML_ELEMENT_SLASH_ && *(p + 1) == _XML_ELEMENT_END_)) {
        return 2;
    }

    if (isquoted == 0x20 && _cc_isspace(*p)) {
        return 1;
    }

    return isquoted == *p ? 1 : 0;
}

_CC_API_PRIVATE(tchar_t*) _XML_parser_attr_value(_cc_sbuf_tchar_t *const buffer) {
    const tchar_t *p = _cc_sbuf_offset(buffer);
    const tchar_t *start = NULL;
    size_t alloc_length = 0;
    size_t skipped_bytes = 0;
    tchar_t *output = NULL;
    tchar_t isquoted = *p;
    int32_t end_tag_len = 0;

    if (_cc_likely(isquoted == _T('"') || isquoted == _T('\''))) {
        start = ++p;
    } else {
        isquoted = 0x20;
        start = p;
    }

    while (*p && ((size_t)(p - buffer->content) < buffer->length) &&
           (end_tag_len = _XML_is_attr_value_end_tag(p, isquoted)) == 0) {
        if (*p == _T('\\') && ((size_t)((p + 1) - buffer->content) < buffer->length)) {
            skipped_bytes++;
            p++;
        }
        p++;
    }

    if (((size_t)(p - buffer->content) >= buffer->length) || end_tag_len == 0) {
        goto XML_FAIL;
    }

    /* This is at most how much we need for the output */
    alloc_length = sizeof(tchar_t) * ((size_t)(p - start) - skipped_bytes + 1);
    output = (tchar_t *)_cc_malloc(alloc_length);

    if (_XML_convert_text(output, alloc_length, start, p)) {
        buffer->offset = (size_t)(p - buffer->content);
        buffer->offset += end_tag_len;
        return output;
    }

XML_FAIL:
    if (output) {
        _cc_free(output);
    }
    return NULL;
}

_CC_API_PRIVATE(int) _XML_attr_read(_cc_rbtree_t *ctx, _cc_sbuf_tchar_t *const buffer) {
    const tchar_t *tmp;

    do {
        tchar_t *name = NULL;
        tchar_t *value = NULL;

        /* check if we skipped to the end of the buffer */
        if (!_XML_buf_skip_whitespace(buffer)) {
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

        /* check if we skipped to the end of the buffer */
        if (!_XML_buf_skip_whitespace(buffer)) {
            return false;
        }

        if (_cc_sbuf_access(buffer) && (*_cc_sbuf_offset(buffer) == '=')) {
            /*
            ** ! skip =
            */
            buffer->offset++;
            /* check if we skipped to the end of the buffer */
            if (!_XML_buf_skip_whitespace(buffer)) {
                return false;
            }
            /*parse the value*/
            value = _XML_parser_attr_value(buffer);
            if (value == NULL) {
                _cc_free(name);
                break;
            }
        }

        if (!_XML_attr_push(ctx, name, value)) {
            _cc_free(value);
            _cc_free(name);
            break;
        }

    } while (_cc_sbuf_access(buffer));

    return 0;
}

_CC_API_PRIVATE(bool_t) _XML_child_read(_cc_xml_t *ctx, _cc_sbuf_tchar_t *const buffer, int32_t depth) {
    _cc_xml_t *item;
    do {
        int tailed = 0;
        const tchar_t *p;

        if (!_XML_buf_skip_whitespace(buffer)) {
            return true;
        }

        p = _cc_sbuf_offset(buffer);

        if (*p == _XML_ELEMENT_START_ && *(p + 1) == '/') {
            return true;
        }

        item = (_cc_xml_t *)_cc_malloc(sizeof(_cc_xml_t));
        _XML_NODE_INIT(item, _CC_XML_NULL_);
        _cc_list_iterator_push(&ctx->element.uni_child, &item->lnk);

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
                    if (!_XML_parser_text(buffer, &item->element.uni_context)) {
                        return false;
                    }
                    item->type = _CC_XML_CONTEXT_;
                } else if (_tcsncmp(_T("DOCTYPE"), p + 2, 7) == 0) {
                    buffer->offset += 9;
                    /* check if we skipped to the end of the buffer */
                    if (!_XML_buf_skip_whitespace(buffer)) {
                        return false;
                    }
                    if(!_XML_parser_doctype(buffer, &item->element.uni_doctype)) {
                        return false;
                    }
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
                _cc_list_iterator_cleanup(&item->element.uni_child);

                if (!_XML_child_read(item, buffer, depth + 1)) {
                    return false;
                }

                if (_cc_list_iterator_empty(&item->element.uni_child)) {
                    item->type = _CC_XML_NULL_;
                }

                if (!_XML_buf_skip_whitespace(buffer)) {
                    return false;
                }

                if (*_cc_sbuf_offset(buffer) == _XML_ELEMENT_START_ &&
                    *_cc_sbuf_offset_at(buffer, 1) == _XML_ELEMENT_SLASH_) {
                    tchar_t end_tag[128];
                    int32_t end_tag_lengtn = _sntprintf(end_tag, _cc_countof(end_tag), _T("</%s"), item->name);
                    if (_tcsncmp(_cc_sbuf_offset(buffer), end_tag, end_tag_lengtn) != 0) {
                        return false;
                    }
                    /* skip </item-name */
                    buffer->offset += end_tag_lengtn;

                    if (!_XML_buf_skip_whitespace(buffer)) {
                        return false;
                    }
                    if (*_cc_sbuf_offset(buffer) != _XML_ELEMENT_END_) {
                        return false;
                    }
                    /* skip > */
                    buffer->offset++;
                }
            }
        } else {
            item->type = _CC_XML_CONTEXT_;
            item->element.uni_context.cdata = 0;
            if (!_XML_parser_text(buffer, &item->element.uni_context)) {
                return false;
            }
        }
    } while (_cc_sbuf_access(buffer));

    return true;
}

_CC_API_PRIVATE(bool_t) _XML_read(_cc_xml_t *ctx, _cc_sbuf_tchar_t *const buffer) {
    if (*_cc_sbuf_offset(buffer) == _XML_ELEMENT_START_ && *_cc_sbuf_offset_at(buffer, 1) == '?') {
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

_CC_API_PUBLIC(_cc_xml_t*) _cc_xml_parser(_cc_sbuf_tchar_t *const buffer) {
    _cc_xml_t *item = NULL;
    _cc_XML_error_t local_error;

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
    _cc_global_XML_error = local_error;

    _cc_destroy_xml(&item);
    return NULL;
}

_CC_API_PUBLIC(const tchar_t*) _cc_xml_error(void) {
    return (_cc_global_XML_error.content + _cc_global_XML_error.position);
}

_CC_API_PUBLIC(_cc_xml_t*) _cc_open_xml_file(const tchar_t *file_name) {
    _cc_sbuf_tchar_t buffer;
    _cc_xml_t *item = NULL;

    byte_t *content = NULL;
    size_t offset = 0;

    _cc_buf_t *buf = _cc_load_buf(file_name);

    if (_cc_unlikely(buf == NULL)) {
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

    item = _cc_xml_parser(&buffer);

    _cc_destroy_buf(&buf);

    return item;
}

_CC_API_PUBLIC(_cc_xml_t*) _cc_parse_xml(const tchar_t *src) {
    _cc_sbuf_tchar_t buffer;
    buffer.content = src;
    buffer.length = _tcslen(src);
    buffer.offset = 0;
    buffer.line = 1;
    buffer.depth = 0;

    return _cc_xml_parser(&buffer);
}
