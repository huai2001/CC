#include "json.c.h"

int32_t _json_get_object(_cc_rbtree_iterator_t *left, uintptr_t keyword) {
    _cc_json_t *_left = _cc_upcast(left, _cc_json_t, lnk);

    return _tcscmp(_left->name, (const tchar_t *)keyword);
}

_CC_API_PUBLIC(_cc_json_t*) _cc_json_alloc_object(byte_t type, const tchar_t *keyword) {
    _cc_json_t *item = (_cc_json_t *)_cc_malloc(sizeof(_cc_json_t));
    bzero(item, sizeof(_cc_json_t));
    item->type = type;
    item->element.uni_object.rb_node = NULL;
    if (keyword) {
        item->name = _cc_sds_alloc(keyword,_tcslen(keyword));
    } else {
        item->name = NULL;
    }

    return item;
}

bool_t _json_object_push(_cc_json_t *ctx, _cc_json_t *item, bool_t replacement) {
    _cc_rbtree_t *root;
    _cc_rbtree_iterator_t **node;
    _cc_rbtree_iterator_t *parent = NULL;
    _cc_json_t *curr;
    int32_t result;

    _cc_assert(ctx != NULL);
    _cc_assert(item != NULL);
    
    root = &ctx->element.uni_object;
    node = &(root->rb_node);

    while (*node) {
        curr = _cc_upcast((*node), _cc_json_t, lnk);
        result = _tcscmp(curr->name, item->name);

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
_CC_API_PUBLIC(bool_t) _cc_json_object_push(_cc_json_t *ctx, _cc_json_t *item, bool_t replacement) {
    if (_cc_unlikely(ctx == NULL || item == NULL)) {
        return false;
    }

    if (_cc_unlikely(ctx == item)) {
        return true;
    }

    if (ctx->type == _CC_JSON_ARRAY_) {
        _json_array_push(ctx, item);
        return true;
    }

    if (ctx->type == _CC_JSON_OBJECT_) {
        return _json_object_push(ctx,item,replacement);
    }

    return false;
}


/**/
_CC_API_PUBLIC(bool_t) _cc_json_object_remove(_cc_json_t *ctx, const tchar_t *keyword) {
    _cc_rbtree_iterator_t *node;
    if (ctx->type != _CC_JSON_OBJECT_) {
        return false;
    }

    node = _cc_rbtree_get(&ctx->element.uni_object, (uintptr_t)keyword, _json_get_object);
    if (node == NULL) {
        return false;
    }

    _cc_rbtree_remove(&ctx->element.uni_object, node);
    _json_free_object_rb_node(node);
    return true;
}
