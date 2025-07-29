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
#include "xml.c.h"

_CC_API_PRIVATE(_cc_xml_attr_t*) _XML_attr_alloc(_cc_xml_attr_t *ctx, tchar_t *name, tchar_t *value) {
    if (ctx == nullptr) {
        ctx = (_cc_xml_attr_t *)_cc_malloc(sizeof(_cc_xml_attr_t));
        ctx->name = nullptr;
        ctx->value = nullptr;
    }

    if (name) {
        if (ctx->name) {
            _cc_free((pvoid_t)ctx->name);
        }
        ctx->name = name;
    }

    if (ctx->value) {
        _cc_free((pvoid_t)ctx->value);
    }

    ctx->value = value;

    return ctx;
}

/*
 * Utility to jump whitespace and cr/lf
 */
bool_t _XML_jump_whitespace(_cc_sbuf_t *const buffer) {
    register const tchar_t *ptr = 0;
    if (_cc_unlikely((buffer == nullptr) || (buffer->content == nullptr))) {
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
bool_t _XML_attr_push(_cc_rbtree_t *ctx, tchar_t *name, tchar_t *value) {
    int32_t result = 0;
    _cc_xml_attr_t *item = nullptr;
    _cc_rbtree_iterator_t **node = &(ctx->rb_node), *parent = nullptr;

    while (*node) {
        item = _cc_upcast(*node, _cc_xml_attr_t, lnk);
        result = _tcscmp(name, item->name);

        parent = *node;

        if (result < 0) {
            node = &((*node)->left);
        } else if (result > 0) {
            node = &((*node)->right);
        } else {
            _XML_attr_alloc(item, name, value);
            return true;
        }
    }

    item = _XML_attr_alloc(nullptr, name, value);
    if (item == nullptr) {
        return false;
    }

    _cc_rbtree_insert(ctx, &item->lnk, parent, node);
    return true;
}

/**/
_CC_API_PUBLIC(_cc_xml_t*) _cc_new_xml_element(byte_t type) {
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
_CC_API_PUBLIC(const tchar_t*) _cc_xml_element_text(_cc_xml_t *ctx) {
    if (ctx && !_cc_list_iterator_empty(&ctx->element.uni_child)) {
        _cc_xml_t *item = _cc_upcast(ctx->element.uni_child.next, _cc_xml_t, lnk);
        if (item->type == _CC_XML_CONTEXT_) {
            return item->element.uni_context.text;
        }
    }
    return nullptr;
}

/**/
_CC_API_PRIVATE(int32_t) _XML_attr_find(_cc_rbtree_iterator_t *iter, pvoid_t args) {
    _cc_xml_attr_t *item = _cc_upcast(iter, _cc_xml_attr_t, lnk);
    return _tcscmp((const tchar_t *)args, item->name);
}

/**/
_CC_API_PUBLIC(const tchar_t*) _cc_xml_element_attr_find(_cc_xml_t *ctx, const tchar_t *keyword) {
    if (ctx && ctx->attr.rb_node != nullptr) {
        _cc_rbtree_iterator_t *item = _cc_rbtree_get(&ctx->attr, (pvoid_t)keyword, _XML_attr_find);
        if (item) {
            _cc_xml_attr_t *attr = _cc_upcast(item, _cc_xml_attr_t, lnk);
            return attr->value;
        }
    }
    return nullptr;
}

_CC_API_PUBLIC(bool_t) _cc_xml_element_set_attr(_cc_xml_t *ctx, const tchar_t *keyword, const tchar_t *fmt, ...) {
    tchar_t buf[1024];
    _cc_assert(fmt != nullptr);

    if (nullptr != _tcschr((tchar_t *)fmt, '%')) {
        va_list args;
        va_start(args, fmt);
        _vsntprintf(buf, _cc_countof(buf), fmt, args);
        va_end(args);

        fmt = buf;
    }

    return _XML_attr_push(&ctx->attr, _cc_tcsdup(keyword), _cc_tcsdup(fmt));
}

/**/
_CC_API_PUBLIC(_cc_xml_t*) _cc_xml_element_first_child(_cc_xml_t *ctx) {
    if (ctx->type != _CC_XML_CHILD_ || _cc_list_iterator_empty(&ctx->element.uni_child)) {
        return nullptr;
    }

    return _cc_upcast(ctx->element.uni_child.next, _cc_xml_t, lnk);
}

/**/
_CC_API_PUBLIC(_cc_xml_t*) _cc_xml_element_next_child(_cc_xml_t *ctx) {
    if (ctx->lnk.next == &ctx->lnk) {
        return nullptr;
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
    return nullptr;
}

/**/
_CC_API_PUBLIC(_cc_xml_t*) _cc_xml_element_find(_cc_xml_t *ctx, tchar_t *item) {
    tchar_t *p;
    tchar_t *pp = item;

    if (ctx->type != _CC_XML_CHILD_) {
        return nullptr;
    }

    while ((p = _tcschr(pp, '/'))) {
        ctx = XML_find(ctx, pp, (size_t)(p - pp));
        if (ctx == nullptr) {
            return nullptr;
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
        _cc_free(p->name);
    }

    if (p->value) {
        _cc_free(p->value);
    }
    _cc_free(p);
}

/**/
static void _xml_free(_cc_xml_t *ctx) {
    if (ctx->name) {
        _cc_free(ctx->name);
    }

    _cc_rbtree_destroy(&ctx->attr, _xml_free_attr_rb_node);

    switch (ctx->type) {
    case _CC_XML_COMMENT_:
        if (ctx->element.uni_comment) {
            _cc_free(ctx->element.uni_comment);
            ctx->element.uni_comment = nullptr;
        }
        break;

    case _CC_XML_CONTEXT_:
        if (ctx->element.uni_context.text) {
            _cc_free(ctx->element.uni_context.text);
            ctx->element.uni_context.text = nullptr;
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
static void _dump_xml_buffer(_cc_xml_t *XML, int32_t depth, _cc_buf_t *buf) {
    tchar_t depth_buf[1024] = {0};
    int32_t i = 0;
    _cc_list_iterator_t *v = nullptr;

    depth &= 1023;
    for (i = 0; i < depth; i++) {
        depth_buf[i] = _T('\t');
    }

    _cc_buf_append(buf, (byte_t *)depth_buf, i * sizeof(tchar_t));

    if (XML->type == _CC_XML_COMMENT_) {
        _cc_buf_appendf(buf, _T("<!-- %s -->\n"), XML->element.uni_comment);
        return;
    } else if (XML->type == _CC_XML_CONTEXT_) {
        if (XML->element.uni_context.cdata) {
            _cc_buf_appendf(buf, _T("<![CDATA[%s]]>\n"), XML->element.uni_context.text);
        } else {
            _cc_buf_appendf(buf, _T("%s\n"), XML->element.uni_context.text);
        }
        return;
    } else if (XML->type == _CC_XML_DOCTYPE_) {
        _cc_buf_appendf(buf, _T("<!DOCTYPE %s/>\n"), XML->element.uni_doctype);
        return;
    }

    if (XML->name) {
        _cc_buf_appendf(buf, _T("<%s"), XML->name);
        _cc_rbtree_for_each(rbnode, &XML->attr, {
            _cc_xml_attr_t *p = _cc_upcast(rbnode, _cc_xml_attr_t, lnk);
            _cc_buf_appendf(buf, _T(" %s=\"%s\""), p->name, p->value);
        });

        if (XML->type == _CC_XML_NULL_) {
            _cc_buf_puts(buf, _T(" />\n"));
            return;
        }

        _cc_buf_puts(buf, _T(">\n"));

        _cc_list_iterator_for(v, &XML->element.uni_child) {
            _dump_xml_buffer(_cc_upcast(v, _cc_xml_t, lnk), depth + 1, buf);
        }

        _cc_buf_append(buf, (byte_t *)depth_buf, i * sizeof(tchar_t));
        _cc_buf_appendf(buf, _T("</%s>\n"), XML->name);
    }
}

/**/
_CC_API_PUBLIC(void) _cc_dump_xml(_cc_xml_t *XML,_cc_buf_t *buf) {
    _cc_list_iterator_t *v;
    _cc_alloc_buf(buf,_CC_16K_BUFFER_SIZE_);
    _cc_buf_puts(buf, _T("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"));
    _cc_list_iterator_for(v, &XML->element.uni_child) {
        _dump_xml_buffer(_cc_upcast(v, _cc_xml_t, lnk), 0, buf);
    }
}
