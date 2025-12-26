#include "xml.c.h"
/*
 * Utility to jump whitespace and cr/lf
 */
bool_t _XML_jump_whitespace(_cc_sbuf_t *const buffer) {
    register const tchar_t *ptr = 0;
    if (_cc_unlikely((buffer == NULL) || (buffer->content == NULL))) {
        return false;
    }

    while (_cc_sbuf_access(buffer)) {
        ptr = _cc_sbuf_offset(buffer);
        /*Whitespace characters.*/
        if (_cc_isspace(*ptr)) {
            if (*ptr == _T(_CC_LF_)) {
                buffer->line++;
            }
            buffer->offset++;
            continue;
        }
        break;
    }

    return _cc_sbuf_access(buffer);
}

/**/
bool_t _XML_attr_push(_cc_rbtree_t *ctx, _cc_sds_t name, _cc_sds_t value) {
    int32_t result = 0;
    _cc_xml_attr_t *item = NULL;
    _cc_rbtree_iterator_t **node = &(ctx->rb_node), *parent = NULL;

    while (*node) {
        item = _cc_upcast(*node, _cc_xml_attr_t, lnk);
        result = _tcscmp(name, item->name);

        parent = *node;

        if (result < 0) {
            node = &((*node)->left);
        } else if (result > 0) {
            node = &((*node)->right);
        } else {
            _cc_sds_free(name);
            if (item->value) {
                _cc_sds_free(item->value);
            }
            item->value = value;
            return true;
        }
    }

    item = (_cc_xml_attr_t *)_cc_malloc(sizeof(_cc_xml_attr_t));
    item->name = name;
    item->value = value;
    _cc_rbtree_insert(ctx, &item->lnk, parent, node);
    return true;
}

/**/
_CC_API_PUBLIC(_cc_xml_t*) _cc_alloc_xml_element(byte_t type) {
    _cc_xml_t *xml = (_cc_xml_t *)_cc_malloc(sizeof(_cc_xml_t));
    _XML_NODE_INIT(xml, type);

    return xml;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_xml_element_append(_cc_xml_t *ctx, _cc_xml_t *child) {
    if (ctx->type == _CC_XML_NULL_) {
        ctx->type = _CC_XML_CHILD_;
    } else if (ctx->type != _CC_XML_CHILD_) {
        return false;
    }

    _cc_list_iterator_push(&ctx->element.uni_child, &child->lnk);
    return true;
}
/**/
_CC_API_PUBLIC(const _cc_sds_t) _cc_xml_element_text(_cc_xml_t *ctx) {
    if (ctx && !_cc_list_iterator_empty(&ctx->element.uni_child)) {
        _cc_xml_t *item = _cc_upcast(ctx->element.uni_child.next, _cc_xml_t, lnk);
        if (item->type == _CC_XML_CONTEXT_) {
            return item->element.uni_context.text;
        }
    }
    return NULL;
}

/**/
_CC_API_PRIVATE(int32_t) _XML_attr_find(_cc_rbtree_iterator_t *iter, uintptr_t args) {
    _cc_xml_attr_t *item = _cc_upcast(iter, _cc_xml_attr_t, lnk);
    return _tcscmp((const tchar_t *)args, item->name);
}

/**/
_CC_API_PUBLIC(const _cc_sds_t) _cc_xml_element_attr(_cc_xml_t *ctx, const tchar_t *keyword) {
    if (ctx && ctx->attr.rb_node != NULL) {
        _cc_rbtree_iterator_t *item = _cc_rbtree_get(&ctx->attr, (uintptr_t)keyword, _XML_attr_find);
        if (item) {
            _cc_xml_attr_t *attr = _cc_upcast(item, _cc_xml_attr_t, lnk);
            return attr->value;
        }
    }
    return NULL;
}

_CC_API_PUBLIC(bool_t) _cc_xml_element_set_attr(_cc_xml_t *ctx, const tchar_t *keyword, const tchar_t *fmt, ...) {
    tchar_t buf[1024];
    size_t length;
    _cc_assert(fmt != NULL);

    if (NULL != _tcschr((tchar_t *)fmt, '%')) {
        va_list args;
        va_start(args, fmt);
        length = _vsntprintf(buf, _cc_countof(buf), fmt, args);
        va_end(args);

        fmt = buf;
    } else {
        length = _tcslen(fmt);
    }

    return _XML_attr_push(&ctx->attr, _cc_sds_alloc(keyword,_tcslen(keyword)), _cc_sds_alloc(fmt, length));
}

/**/
_CC_API_PUBLIC(_cc_xml_t*) _cc_xml_element_first_child(_cc_xml_t *ctx) {
    if (ctx->type != _CC_XML_CHILD_ || _cc_list_iterator_empty(&ctx->element.uni_child)) {
        return NULL;
    }

    return _cc_upcast(ctx->element.uni_child.next, _cc_xml_t, lnk);
}

/**/
_CC_API_PUBLIC(_cc_xml_t*) _cc_xml_element_next_child(_cc_xml_t *ctx) {
    if (ctx->lnk.next == &ctx->lnk) {
        return NULL;
    }

    return _cc_upcast(ctx->lnk.next, _cc_xml_t, lnk);
}

_CC_API_PRIVATE(_cc_xml_t*) XML_find(_cc_xml_t *ctx, tchar_t *name, size_t len) {
    _cc_list_iterator_t *v;
    _cc_list_iterator_for(v, &ctx->element.uni_child) {
        _cc_xml_t *item = _cc_upcast(v, _cc_xml_t, lnk);
        if (item->type != _CC_XML_CHILD_) {
            continue;
        }

        if (item->name && _tcsncmp(name, item->name, len) == 0) {
            return item;
        }
    }
    return NULL;
}

/**/
_CC_API_PUBLIC(_cc_xml_t*) _cc_xml_element_find(_cc_xml_t *ctx, tchar_t *name) {
    tchar_t *p;
    tchar_t *pp = name;

    if (ctx->type != _CC_XML_CHILD_) {
        return NULL;
    }

    while ((p = _tcschr(pp, '/'))) {
        ctx = XML_find(ctx, pp, (size_t)(p - pp));
        if (ctx == NULL) {
            return NULL;
        }
        pp = p + 1;
    }

    if (*pp != 0) {
        ctx = XML_find(ctx, pp, _tcslen(pp));
    }

    return ctx;
}

/**/
_CC_API_PRIVATE(void) _xml_free_attr_rb_node(_cc_rbtree_iterator_t *node) {
    _cc_xml_attr_t *p = _cc_upcast(node, _cc_xml_attr_t, lnk);
    if (p->name) {
        _cc_sds_free(p->name);
    }

    if (p->value) {
        _cc_sds_free(p->value);
    }
    _cc_free(p);
}

/**/
static void _xml_free(_cc_xml_t *ctx) {
    if (ctx->name) {
        _cc_sds_free(ctx->name);
    }

    _cc_rbtree_destroy(&ctx->attr, _xml_free_attr_rb_node);

    switch (ctx->type) {
    case _CC_XML_COMMENT_:
        if (ctx->element.uni_comment) {
            _cc_sds_free(ctx->element.uni_comment);
            ctx->element.uni_comment = NULL;
        }
        break;

    case _CC_XML_CONTEXT_:
        if (ctx->element.uni_context.text) {
            _cc_sds_free(ctx->element.uni_context.text);
            ctx->element.uni_context.text = NULL;
        }
        break;

    case _CC_XML_CHILD_:
        _cc_list_iterator_for_each(v, &ctx->element.uni_child, { 
            _xml_free(_cc_upcast(v, _cc_xml_t, lnk));
        });
        _cc_list_iterator_cleanup(&ctx->element.uni_child);
        break;
    }
    _cc_free(ctx);
}

/**/
_CC_API_PUBLIC(void) _cc_free_xml(_cc_xml_t *ctx) {
    _xml_free(ctx);
}

/**/
_CC_API_PUBLIC(const tchar_t*) _cc_xml_error(void) {
    return _cc_get_syntax_error();
}

/**/
static void _dump_xml_buffer(const _cc_xml_t *XML, _cc_buf_t *buf) {
    _cc_list_iterator_t *v = NULL;

    if (XML->type == _CC_XML_COMMENT_) {
        _cc_buf_appendf(buf, _T("<!-- %s -->"), XML->element.uni_comment);
        return;
    } else if (XML->type == _CC_XML_CONTEXT_) {
        if (XML->element.uni_context.cdata) {
            _cc_buf_appendf(buf, _T("<![CDATA[%s]]>"), XML->element.uni_context.text);
        } else {
            _cc_buf_append(buf, XML->element.uni_context.text,_cc_sds_length(XML->element.uni_context.text) - 1 * sizeof(tchar_t));
        }
        return;
    } else if (XML->type == _CC_XML_DOCTYPE_) {
        _cc_buf_appendf(buf, _T("<!DOCTYPE %s/>"), XML->element.uni_doctype);
        return;
    }

    if (XML->name) {
        _cc_buf_appendf(buf, _T("<%s"), XML->name);
        _cc_rbtree_for_each(rbnode, &XML->attr, {
            _cc_xml_attr_t *p = _cc_upcast(rbnode, _cc_xml_attr_t, lnk);
            _cc_buf_appendf(buf, _T(" %s=\"%s\""), p->name, p->value);
        });

        if (XML->type == _CC_XML_NULL_) {
            _cc_buf_puts(buf, _T(" />"));
            return;
        }

        _cc_buf_putchar(buf, _T('>'));
        
        _cc_list_iterator_for(v, &XML->element.uni_child) {
            _dump_xml_buffer(_cc_upcast(v, _cc_xml_t, lnk), buf);
        }
        _cc_buf_appendf(buf, _T("</%s>"), XML->name);
    }
}

/**/
_CC_API_PUBLIC(void) _cc_dump_xml(const _cc_xml_t *XML,_cc_buf_t *buf) {
    _cc_list_iterator_t *v;
    _cc_alloc_buf(buf,_CC_16K_BUFFER_SIZE_);
    _cc_buf_puts(buf, _T("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"));
    _cc_list_iterator_for(v, &XML->element.uni_child) {
        _dump_xml_buffer(_cc_upcast(v, _cc_xml_t, lnk), buf);
    }
}
