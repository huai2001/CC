#ifndef _C_CC_RB_TREE_H_INCLUDED_
#define _C_CC_RB_TREE_H_INCLUDED_

#include "cores.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**/
#define _cc_rbtree_for_each(__VAL, __FIRST, __OP)                           \
    do {                                                                    \
        _cc_rb_t *__VAL, *__NEXT = _cc_rbtree_first(__FIRST);               \
        while (NULL != __NEXT) {                                            \
            __VAL = __NEXT;                                                 \
            __NEXT = _cc_rb_next(__NEXT);                                   \
            __OP                                                            \
        }                                                                   \
    } while (0)

/**/
#define _cc_rbtree_for_backward(__CURR, __FIRST)                            \
    for (__CURR = _cc_rbtree_last(__FIRST); __CURR != NULL;                 \
         __CURR = _cc_rb_prev(__CURR))

#define _cc_rbtree_for_forward(__CURR, __FIRST)                             \
    for (__CURR = _cc_rbtree_first(__FIRST); __CURR != NULL;                \
         __CURR = _cc_rb_next(__CURR))

#define _cc_rbtree_for _cc_rbtree_for_forward

enum { _CC_RB_RED_ = 0, _CC_RB_BLACK_ };

struct _cc_rb {
    // since 2006 parent_color holds both parent ptr + color in otherwise
    // unused low 2 bits
    // the kernal guarantees 'uintptr_t' same size as 'ptr' at compile time
    // but here we assume it works until it doesn't
    uintptr_t parent_color;
    struct _cc_rb *right;
    struct _cc_rb *left;
};

typedef struct _cc_rb _cc_rb_t;

/* The alignment might seem pointless, but allegedly CRIS needs it */
struct _cc_rbtree {
    _cc_rb_t *rb_node;
};

typedef struct _cc_rbtree _cc_rbtree_t;

#define _cc_rbtree_entry(ptr, type, member) _cc_upcast(ptr, type, member)
#define _CC_RB_EMPTY_ROOT(root) ((root)->rb_node == NULL)

_CC_API_PUBLIC(void) _cc_rbtree_insert_color(_cc_rbtree_t *, _cc_rb_t *);
_CC_API_PUBLIC(void) _cc_rbtree_remove(_cc_rbtree_t *, _cc_rb_t *);
_CC_API_PUBLIC(void) _cc_rbtree_free_all(_cc_rbtree_t *, void (*cb)(_cc_rb_t *));
_CC_API_PUBLIC(void) _cc_rbtree_traverse(_cc_rb_t *node, void (*cb)(_cc_rb_t *, pvoid_t), pvoid_t args);

/* Find logical next and previous nodes in a tree */
_CC_API_PUBLIC(_cc_rb_t *) _cc_rb_next(const _cc_rb_t *);
_CC_API_PUBLIC(_cc_rb_t *) _cc_rb_prev(const _cc_rb_t *);
_CC_API_PUBLIC(_cc_rb_t *) _cc_rbtree_first(const _cc_rbtree_t *);
_CC_API_PUBLIC(_cc_rb_t *) _cc_rbtree_last(const _cc_rbtree_t *);

/* Fast replacement of a single node without remove/rebalance/add/rebalance */
_CC_API_PUBLIC(void)
_cc_rbtree_replace_node(_cc_rbtree_t *root, _cc_rb_t *victim, _cc_rb_t *replacement);

/**/
_CC_API_PUBLIC(_cc_rb_t *) _cc_rbtree_get(const _cc_rbtree_t *root, uintptr_t keyword, int32_t (*cb)(_cc_rb_t *, uintptr_t));
/**/
_CC_API_PUBLIC(bool_t) _cc_rbtree_push(_cc_rbtree_t *root, _cc_rb_t *data, int32_t (*cb)(_cc_rb_t *, _cc_rb_t *));

/**/
_CC_FORCE_INLINE_ void _cc_rbtree_cleanup(_cc_rbtree_t *root) {
    root->rb_node = NULL;
}

/**/
_CC_FORCE_INLINE_ void _cc_rb_cleanup(_cc_rb_t *rb) {
    rb->right = NULL;
    rb->left = NULL;
    rb->parent_color = 0;
}

/**/
_CC_FORCE_INLINE_ void _cc_rbtree_insert(_cc_rbtree_t *root, _cc_rb_t *node, _cc_rb_t *parent, _cc_rb_t **link) {
    node->parent_color = (uintptr_t)parent;
    node->left = node->right = NULL;

    *link = node;

    _cc_rbtree_insert_color(root, node);
}

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#endif /* _C_CC_RB_TREE_H_INCLUDED_ */
