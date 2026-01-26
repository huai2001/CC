#ifndef _C_CC_LIST_ITERATOR_H_INCLUDED_
#define _C_CC_LIST_ITERATOR_H_INCLUDED_

#include "os.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define _cc_list_next(x) (x)->next
#define _cc_list_prev(x) (x)->prev

#define _cc_list_for(__NEXT, __FIRST) \
    for (__NEXT = (__FIRST)->next; __NEXT != (__FIRST); __NEXT = __NEXT->next)

/**/
#define _cc_list_for_each(__CURR, __FIRST, __OP)        \
    do {                                                         \
        _cc_list_t* __NEXT = (__FIRST)->next;           \
        _cc_list_t* __CURR;                             \
        while (__NEXT != (__FIRST)) {                            \
            __CURR = __NEXT;                                     \
            __NEXT = __NEXT->next;                               \
            __OP                                                 \
        }                                                        \
    } while (0)

/**/
#define _cc_list_for_each_prev(__CURR, __FIRST, __OP)   \
    do {                                                         \
        _cc_list_t* __PREV = (__FIRST)->prev;           \
        _cc_list_t* __CURR;                             \
        while (__PREV != (__FIRST)) {                            \
            __CURR = __PREV;                                     \
            __PREV = __PREV->prev;                               \
            __OP                                                 \
        }                                                        \
    } while (0)
/**/
typedef struct _cc_list _cc_list_t;

/**/
struct _cc_list {
    _cc_list_t *prev;
    _cc_list_t *next;
};

/**/
_CC_FORCE_INLINE_ _cc_list_t *_cc_list_first(_cc_list_t *lnk) {
    return lnk->next;
}

/**/
_CC_FORCE_INLINE_ _cc_list_t *_cc_list_last(_cc_list_t *lnk) {
    return lnk->prev;
}

/**/
_CC_FORCE_INLINE_ void _cc_list_cleanup(_cc_list_t *lnk) {
    lnk->prev = lnk;
    lnk->next = lnk;
}

/**/
_CC_FORCE_INLINE_ bool_t _cc_list_empty(_cc_list_t *lnk) {
    _cc_assert(lnk != NULL);
    return (lnk->next == lnk || lnk->next == NULL);
}

/**/
_CC_FORCE_INLINE_ void _cc_list_insert(_cc_list_t *lnk, _cc_list_t *prev,
        _cc_list_t *next) {
    lnk->next = next;
    lnk->prev = prev;
    next->prev = lnk;
    prev->next = lnk;
}

/**/
_CC_FORCE_INLINE_ void _cc_list_delete(_cc_list_t *prev, _cc_list_t *next) {
    prev->next = next;
    next->prev = prev;
}

/**/
_CC_FORCE_INLINE_ void _cc_list_remove(_cc_list_t *lnk) {
    _cc_list_delete(lnk->prev, lnk->next);
    _cc_list_cleanup(lnk);
}

/**/
_CC_FORCE_INLINE_ void _cc_list_push_front(_cc_list_t *head, _cc_list_t *lnk) {
    _cc_assert(head != NULL);
    _cc_assert(head->next != NULL);
    _cc_list_insert(lnk, head, head->next);
}

/**/
_CC_FORCE_INLINE_ void _cc_list_push_back(_cc_list_t *head, _cc_list_t *lnk) {
    _cc_assert(head != NULL);
    _cc_assert(head->prev != NULL);
    _cc_list_insert(lnk, head->prev, head);
}

/**/
_CC_FORCE_INLINE_ void _cc_list_push(_cc_list_t *head, _cc_list_t *lnk) {
    _cc_list_push_back(head, lnk);
}

/**/
_CC_FORCE_INLINE_ _cc_list_t *_cc_list_pop_front(_cc_list_t *lnk) {
    if (_cc_list_empty(lnk)) {
        return lnk;
    }

    lnk = lnk->prev;
    _cc_list_remove(lnk);
    return lnk;
}

/**/
_CC_FORCE_INLINE_ _cc_list_t *_cc_list_pop_back(_cc_list_t *lnk) {
    if (_cc_list_empty(lnk)) {
        return lnk;
    }

    lnk = lnk->next;
    _cc_list_remove(lnk);
    return lnk;
}

/**/
_CC_FORCE_INLINE_ _cc_list_t *_cc_list_pop(_cc_list_t *lnk) {
    return _cc_list_pop_front(lnk);
}

/**/
_CC_FORCE_INLINE_ void _cc_list_swap(_cc_list_t *head, _cc_list_t *lnk) {
    if (!_cc_list_empty(lnk)) {
        _cc_list_delete(lnk->prev, lnk->next);
    }
    _cc_list_push_front(head, lnk);
}

/**/
_CC_API_PUBLIC(void) _cc_list_append(_cc_list_t *head, _cc_list_t *add);

/* Return the element at the specified zero-based index
 * where 0 is the head, 1 is the element next to head
 * and so on. Negative integers are used in order to count
 * from the tail, -1 is the last element, -2 the penultimate
 * and so on. If the index is out of range NULL is returned. */
_CC_API_PUBLIC(_cc_list_t *) _cc_list_index(_cc_list_t *lnk, long index);

/* the stable insertion sort */
_CC_API_PUBLIC(void) _cc_list_sort(_cc_list_t *lnk, int32_t (*_cmp)(const _cc_list_t *, const _cc_list_t *));

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_LIST_ITERATOR_H_INCLUDED_*/
