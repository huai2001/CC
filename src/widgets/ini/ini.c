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
#include "ini.c.h"

static void _ini_free_rb_node(_cc_rbtree_iterator_t* node) {
    _ini_free(_cc_upcast(node, _cc_ini_t, lnk));
}

_CC_API_PUBLIC(_cc_ini_t*) _INI_alloc(int type) {
    _cc_ini_t *ctx = (_cc_ini_t*)_cc_malloc(sizeof(_cc_ini_t));
    bzero(ctx, sizeof(_cc_ini_t));
    ctx->type = type;
    ctx->element.uni_object.rb_node = nullptr;
    ctx->length = 0;
    
    return ctx;
}

_CC_API_PUBLIC(void) _ini_free(_cc_ini_t* p) {
    if (p->name) {
        _cc_free(p->name);
    }
    switch(p->type) {
    case _CC_INI_SECTION_:
        _cc_rbtree_destroy(&p->element.uni_object, _ini_free_rb_node);
        break;
    case _CC_INI_STRING_:
        if (p->element.uni_string) {
            _cc_free(p->element.uni_string);
        }
        break;
    }
    _cc_free(p);
}

_CC_API_PRIVATE(int32_t) _INI_get(_cc_rbtree_iterator_t* v, pvoid_t args) {
    _cc_ini_t* element = _cc_upcast(v, _cc_ini_t, lnk);
    return _tcscmp((const tchar_t*)args, element->name);
}

_CC_API_PUBLIC(_cc_ini_t*) _INI_push(_cc_rbtree_t* root, tchar_t *name, int type) {
    _cc_rbtree_iterator_t **node;
    _cc_rbtree_iterator_t *parent = NULL;
    _cc_ini_t* item;
    int32_t result;

    node = &(root->rb_node);

    while (*node) {
        item = _cc_upcast(*node, _cc_ini_t, lnk);
        result = _tcscmp(name, item->name);

        parent = *node;

        if (result < 0) {
            node = &((*node)->left);
        } else if (result > 0) {
            node = &((*node)->right);
        } else {
            _cc_free(name);
            return item;
        }
    }
    item = _INI_alloc(type);
    item->name = name;
    _cc_rbtree_insert(root, &item->lnk, parent, node);
    return item;
}

/**/
_CC_API_PUBLIC(_cc_ini_t*) _cc_ini_find(_cc_ini_t* item, const tchar_t* name) {
    _cc_rbtree_iterator_t* node;

    node = _cc_rbtree_get(&item->element.uni_object, (pvoid_t)name, _INI_get);
    if (node == NULL) {
        return NULL;
    }
    return _cc_upcast(node, _cc_ini_t, lnk);
}
/**/
_CC_API_PUBLIC(const tchar_t*) _cc_ini_find_string(_cc_ini_t* item, const tchar_t* name) {
    _cc_ini_t* node = _cc_ini_find(item, name);

    if (node->type == _CC_INI_STRING_) {
        return node->element.uni_string;
    }
    return nullptr;
}
/**/
_CC_API_PUBLIC(void) _cc_destroy_ini(_cc_ini_t** ctx) {
    
    if (_cc_unlikely(ctx == nullptr || *ctx == nullptr)) {
        return;
    }

    if ((*ctx)->type == _CC_INI_SECTION_) {
        _cc_rbtree_destroy(&(*ctx)->element.uni_object, _ini_free_rb_node);
    }

    _cc_free(*ctx);
    *ctx = nullptr;
}

/**/
_CC_API_PUBLIC(const tchar_t*) _cc_ini_error(void) {
    return _cc_get_syntax_error();
}

static void _INI_dump(_cc_buf_t* buf, _cc_rbtree_t* rb) {
    _cc_rbtree_for_each(v, rb, {
        _cc_ini_t* ctx = _cc_upcast(v, _cc_ini_t, lnk);
        switch(ctx->type) {
        case _CC_INI_BOOLEAN_:
            _cc_buf_appendf(buf, _T("%s = %s\n"), ctx->name, ctx->element.uni_boolean?_T("true"):_T("false"));
            break;
        case _CC_INI_STRING_:
            _cc_buf_appendf(buf, _T("%s = \"%s\"\n"), ctx->name, ctx->element.uni_string);
            break;
        case _CC_INI_INT_:
            _cc_buf_appendf(buf, _T("%s = %lld"), ctx->name, (long long)ctx->element.uni_int);
            break;
        case _CC_INI_FLOAT_:
            _cc_buf_appendf(buf, _T("%s = %llf\n"), ctx->name, ctx->element.uni_float);
            break;
        case _CC_INI_SECTION_:
            _cc_buf_appendf(buf, _T("[ %s ]\n"), ctx->name);
            _INI_dump(buf,&ctx->element.uni_object);
            _buf_char_put(buf, _T('\n'));
            break;
        }
        
    });
}

_CC_API_PUBLIC(_cc_buf_t*) _cc_dump_ini(_cc_ini_t* item) {
    _cc_buf_t* buf = _cc_create_buf(_CC_16K_BUFFER_SIZE_);
    _INI_dump(buf, &item->element.uni_object);
    return buf;
}