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

int32_t _json_push_object(_cc_rbtree_iterator_t *left, _cc_rbtree_iterator_t *right) {
    _cc_json_t *_left = _cc_upcast(left, _cc_json_t, lnk);
    _cc_json_t *_right = _cc_upcast(right, _cc_json_t, lnk);

    return _tcscmp(_right->name, _left->name);
}

int32_t _json_get_object(_cc_rbtree_iterator_t *left, pvoid_t args) {
    _cc_json_t *_left = _cc_upcast(left, _cc_json_t, lnk);

    return _tcscmp((const tchar_t *)args, _left->name);
}

void _json_free_object_rb_node(_cc_rbtree_iterator_t *node) {
    _cc_json_t *item = _cc_upcast(node, _cc_json_t, lnk);
    _cc_safe_free(item->name);
    _json_free_node(item);
}

_CC_API_PUBLIC(_cc_json_t*) _cc_json_alloc_object(byte_t type, const tchar_t *keyword) {
    _cc_json_t *item = (_cc_json_t *)_cc_malloc(sizeof(_cc_json_t));
    bzero(item, sizeof(_cc_json_t));
    item->type = type;
    item->size = 0;
    item->length = 0;
    item->element.uni_object.rb_node = nullptr;
    if (keyword) {
        item->name = _cc_tcsdup(keyword);
    } else {
        item->name = nullptr;
    }

    return item;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_json_object_append(_cc_json_t *ctx, _cc_json_t *item, bool_t replacement) {
    _cc_rbtree_t *root;
    int32_t result;
    _cc_rbtree_iterator_t **node;
    _cc_rbtree_iterator_t *parent = nullptr;

    if (_cc_unlikely(ctx == nullptr || item == nullptr)) {
        return false;
    }

    if (_cc_unlikely(ctx == item)) {
        return true;
    }

    if (ctx->type == _CC_JSON_ARRAY_) {
        return _json_array_push(ctx, item) != -1;
    }

    if (ctx->type != _CC_JSON_OBJECT_) {
        return false;
    }

    root = &ctx->element.uni_object;
    node = &(root->rb_node);

    while (*node) {
        result = _json_push_object(*node, &item->lnk);

        parent = *node;

        if (result < 0) {
            node = &((*node)->left);
        } else if (result > 0) {
            node = &((*node)->right);
        } else {
            if (replacement) {
                _cc_rbtree_replace_node(root, parent, &item->lnk);
                /*free old json object*/
                _json_free_object_rb_node(parent);
                return true;
            }
            return false;
        }
    }

    _cc_rbtree_insert(root, &item->lnk, parent, node);
    return true;
}


/**/
_CC_API_PUBLIC(bool_t) _cc_json_object_remove(_cc_json_t *ctx, const tchar_t *keyword) {
    _cc_rbtree_iterator_t *node;
    if (ctx->type != _CC_JSON_OBJECT_) {
        return false;
    }

    node = _cc_rbtree_get(&ctx->element.uni_object, (pvoid_t)keyword, _json_get_object);
    if (node == nullptr) {
        return false;
    }

    _cc_rbtree_erase(&ctx->element.uni_object, node);
    _json_free_object_rb_node(node);
    return true;
}
