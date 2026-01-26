#include "ini.c.h"

static void _ini_free_rb_node(_cc_rb_t* node) {
    _INI_free(_cc_upcast(node, _cc_ini_t, lnk));
}

_cc_ini_t* _INI_alloc(int type) {
    _cc_ini_t *ctx = (_cc_ini_t*)_cc_malloc(sizeof(_cc_ini_t));
    bzero(ctx, sizeof(_cc_ini_t));
    ctx->type = type;
    ctx->element.uni_object.rb_node = NULL;
    
    return ctx;
}

void _INI_free(_cc_ini_t* p) {
    if (p->name) {
        _cc_sds_free(p->name);
    }
    switch(p->type) {
    case _CC_INI_SECTION_:
        _cc_rbtree_free_all(&p->element.uni_object, _ini_free_rb_node);
        break;
    case _CC_INI_STRING_:
        if (p->element.uni_string) {
            _cc_sds_free(p->element.uni_string);
        }
        break;
    }
    _cc_free(p);
}

_cc_ini_t* _INI_push(_cc_rbtree_t* root, _cc_sds_t name, int type) {
    _cc_rb_t **node;
    _cc_rb_t *parent = NULL;
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
            _cc_sds_free(name);
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
    int32_t result = 0;
    _cc_rb_t *node = item->element.uni_object.rb_node;

    while (node) {    
        _cc_ini_t* element = _cc_upcast(node, _cc_ini_t, lnk);
        result = _tcscmp(name, element->name);
        if (result < 0) {
            node = node->left;
        } else if (result > 0) {
            node = node->right;
        } else {
            return element;
        }
    }
    return NULL;
}
/**/
_CC_API_PUBLIC(_cc_sds_t) _cc_ini_find_string(_cc_ini_t* ctx, const tchar_t* name) {
    _cc_ini_t* node = _cc_ini_find(ctx, name);

    if (node->type == _CC_INI_STRING_) {
        return node->element.uni_string;
    }
    return NULL;
}
/**/
_CC_API_PUBLIC(void) _cc_free_ini(_cc_ini_t* ctx) {
    
    if (_cc_unlikely(ctx == NULL)) {
        return;
    }

    if (ctx->type == _CC_INI_SECTION_) {
        _cc_rbtree_free_all(&ctx->element.uni_object, _ini_free_rb_node);
    }

    _cc_free(ctx);
}

/**/
_CC_API_PUBLIC(const tchar_t*) _cc_ini_error(void) {
    return _cc_get_syntax_error();
}

static void _INI_dump(_cc_buf_t* buf, const _cc_rbtree_t* rb) {
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
            _cc_buf_putchar(buf, _T('\n'));
            break;
        }
    });
}

_CC_API_PUBLIC(void) _cc_dump_ini(const _cc_ini_t* ctx, _cc_buf_t* buf) {
    _cc_alloc_buf(buf, _CC_16K_BUFFER_SIZE_);
    _INI_dump(buf, &ctx->element.uni_object);
}