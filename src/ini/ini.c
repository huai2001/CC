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

static _cc_ini_key_t* _INI_alloc(_cc_ini_key_t* ctx,
                                            tchar_t* name,
                                            tchar_t* value) {
    if (ctx == NULL) {
        ctx = (_cc_ini_key_t*)_cc_malloc(sizeof(_cc_ini_key_t));
        ctx->name = NULL;
        ctx->value = NULL;
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

static int32_t _INI_get_section(_cc_rbtree_iterator_t* v,
                                           pvoid_t args) {
    _cc_ini_section_t* section = _cc_upcast(v, _cc_ini_section_t, node);
    return _tcscmp((const tchar_t*)args, section->name);
}

static int32_t _INI_get_section_key(_cc_rbtree_iterator_t* v,
                                               pvoid_t args) {
    _cc_ini_key_t* key = _cc_upcast(v, _cc_ini_key_t, node);
    return _tcscmp((const tchar_t*)args, key->name);
}

bool_t _INI_key_push(_cc_rbtree_t* ctx, tchar_t* name, tchar_t* value) {
    int32_t result = 0;
    _cc_ini_key_t* item = NULL;
    _cc_rbtree_iterator_t **node = &(ctx->rb_node), *parent = NULL;

    while (*node) {
        item = _cc_upcast(*node, _cc_ini_key_t, node);
        result = _tcscmp(name, item->name);

        parent = *node;

        if (result < 0) {
            node = &((*node)->left);
        } else if (result > 0) {
            node = &((*node)->right);
        } else {
            _INI_alloc(item, name, value);
            return true;
        }
    }

    item = _INI_alloc(NULL, name, value);
    if (item == NULL) {
        return false;
    }

    _cc_rbtree_insert(ctx, &item->node, parent, node);
    return true;
}

_cc_ini_section_t* _INI_setion_push(_cc_rbtree_t* ctx, tchar_t* name) {
    int32_t result = 0;
    _cc_ini_section_t* item = NULL;
    _cc_rbtree_iterator_t **node = &(ctx->rb_node), *parent = NULL;

    while (*node) {
        item = _cc_upcast(*node, _cc_ini_section_t, node);
        result = _tcscmp(name, item->name);

        parent = *node;

        if (result < 0) {
            node = &((*node)->left);
        } else if (result > 0) {
            node = &((*node)->right);
        } else {
            return item;
        }
    }

    item = (_cc_ini_section_t*)_cc_malloc(sizeof(_cc_ini_section_t));
    item->name = name;
    _CC_RB_INIT_ROOT(&item->keys);
    _cc_rbtree_insert(ctx, &item->node, parent, node);
    return item;
}

static void _ini_free_rb_node(_cc_rbtree_iterator_t* node) {
    _cc_ini_key_t* p = _cc_upcast(node, _cc_ini_key_t, node);
    if (p->name) {
        _cc_free(p->name);
    }

    if (p->value) {
        _cc_free(p->value);
    }
    _cc_free(p);
}

static void _ini_free_section_rb_node(_cc_rbtree_iterator_t* node) {
    _cc_ini_section_t* ctx = _cc_upcast(node, _cc_ini_section_t, node);
    if (ctx->name) {
        _cc_free(ctx->name);
    }

    _cc_rbtree_destroy(&ctx->keys, _ini_free_rb_node);

    _cc_free(ctx);
}

/**/
bool_t _cc_destroy_ini(_cc_ini_t** ctx) {
    _cc_rbtree_destroy(&(*ctx)->root, _ini_free_section_rb_node);
    *ctx = NULL;
    return true;
}

void _INI_print_key(_cc_buf_t* buf, _cc_rbtree_t* rb) {
    _cc_rbtree_for_each(v, rb, {
        _cc_ini_key_t* ctx = _cc_upcast(v, _cc_ini_key_t, node);
        _cc_buf_puttsf(buf, _T("%s = %s\n"), ctx->name, ctx->value);
    });
}

_cc_buf_t* _cc_print_ini(_cc_ini_t* item) {
    _cc_buf_t* buf = _cc_create_buf(10240);

    _cc_rbtree_for_each(v, &item->root, {
        _cc_ini_section_t* ctx = _cc_upcast(v, _cc_ini_section_t, node);
        _cc_buf_puttsf(buf, _T("[%s]\n"), ctx->name);
        _INI_print_key(buf, &ctx->keys);
        _cc_buf_write(buf, _T("\n"), sizeof(tchar_t));
    });

    return buf;
}

_cc_ini_section_t* _cc_ini_find_section(_cc_ini_t* item,
                                        const tchar_t* section_name) {
    _cc_rbtree_iterator_t* node;

    node = _cc_rbtree_get(&item->root, (pvoid_t)section_name, _INI_get_section);
    if (node == NULL) {
        return NULL;
    }
    return _cc_upcast(node, _cc_ini_section_t, node);
}

const tchar_t* _cc_ini_find_string(_cc_ini_section_t* section,
                                   const tchar_t* key_name) {
    _cc_rbtree_iterator_t* node;
    _cc_ini_key_t* key;

    node =
        _cc_rbtree_get(&section->keys, (pvoid_t)key_name, _INI_get_section_key);
    if (node == NULL) {
        return NULL;
    }
    key = _cc_upcast(node, _cc_ini_key_t, node);
    return key->value;
}

const tchar_t* _cc_ini_find(_cc_ini_t* item,
                            const tchar_t* section_name,
                            const tchar_t* key_name) {
    _cc_rbtree_iterator_t* node;
    _cc_ini_section_t* section;
    _cc_ini_key_t* key;

    node = _cc_rbtree_get(&item->root, (pvoid_t)section_name, _INI_get_section);
    if (node == NULL) {
        return NULL;
    }
    section = _cc_upcast(node, _cc_ini_section_t, node);

    node =
        _cc_rbtree_get(&section->keys, (pvoid_t)key_name, _INI_get_section_key);
    if (node == NULL) {
        return NULL;
    }
    key = _cc_upcast(node, _cc_ini_key_t, node);
    return key->value;
    ;
}
