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
#ifndef _C_CC_LIST_ITERATOR_H_INCLUDED_
#define _C_CC_LIST_ITERATOR_H_INCLUDED_

#include "generic.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define _cc_list_iterator_next(x) (x)->next
#define _cc_list_iterator_prev(x) (x)->prev

#define _cc_list_iterator_for(__NEXT, __FIRST) \
    for (__NEXT = (__FIRST)->next; __NEXT != (__FIRST); __NEXT = __NEXT->next)

#define _cc_list_iterator_for_each _cc_list_iterator_for_each_next
/**/
#define _cc_list_iterator_for_each_next(__CURR, __FIRST, __OP)   \
    do {                                                         \
        _cc_list_iterator_t* __NEXT = (__FIRST)->next;           \
        _cc_list_iterator_t* __CURR;                             \
        while (__NEXT != (__FIRST)) {                \
            __CURR = __NEXT;                                     \
            __NEXT = __NEXT->next;                               \
            __OP                                                 \
        }                                                        \
    } while (0)

/**/
#define _cc_list_iterator_for_each_prev(__CURR, __FIRST, __OP)   \
    do {                                                         \
        _cc_list_iterator_t* __PREV = (__FIRST)->prev;           \
        _cc_list_iterator_t* __CURR;                             \
        while (__PREV != (__FIRST)) {                \
            __CURR = __PREV;                                     \
            __PREV = __PREV->prev;                               \
            __OP                                                 \
        }                                                        \
    } while (0)
/**/
typedef struct _cc_list_iterator _cc_list_iterator_t;

/**/
struct _cc_list_iterator {
    _cc_list_iterator_t *prev;
    _cc_list_iterator_t *next;
};

/**/
_CC_FORCE_INLINE_ _cc_list_iterator_t *_cc_list_iterator_first(_cc_list_iterator_t *lnk) {
    return lnk->next;
}

/**/
_CC_FORCE_INLINE_ _cc_list_iterator_t *_cc_list_iterator_last(_cc_list_iterator_t *lnk) {
    return lnk->prev;
}

/**/
_CC_FORCE_INLINE_ void _cc_list_iterator_cleanup(_cc_list_iterator_t *lnk) {
    lnk->prev = lnk;
    lnk->next = lnk;
}
/**
 * @brief Link empty
 *
 * @param lnk link context
 *
 * @return true if successful or false on error.
 */
_CC_FORCE_INLINE_ bool_t _cc_list_iterator_empty(_cc_list_iterator_t *lnk) {
    _cc_assert(lnk != nullptr);
    return (lnk->next == lnk || lnk->next == nullptr);
}

/*
 * @brief insert link
 *
 * @param lnk New link context
 * @param prev iterator context
 * @param next iterator context
 */
_CC_FORCE_INLINE_ void _cc_list_iterator_insert(_cc_list_iterator_t *lnk, _cc_list_iterator_t *prev,
        _cc_list_iterator_t *next) {
    lnk->next = next;
    lnk->prev = prev;
    next->prev = lnk;
    prev->next = lnk;
}

/*
 * @brief delete link
 *
 * @param prev iterator context
 * @param next iterator context
 */
_CC_FORCE_INLINE_ void _cc_list_iterator_delete(_cc_list_iterator_t *prev, _cc_list_iterator_t *next) {
    prev->next = next;
    next->prev = prev;
}

/**/
_CC_FORCE_INLINE_ void _cc_list_iterator_remove(_cc_list_iterator_t *lnk) {
    _cc_list_iterator_delete(lnk->prev, lnk->next);
    _cc_list_iterator_cleanup(lnk);
}

/**/
_CC_FORCE_INLINE_ void _cc_list_iterator_push_front(_cc_list_iterator_t *head, _cc_list_iterator_t *lnk) {
    _cc_assert(head != nullptr);
    _cc_assert(head->next != nullptr);
    _cc_list_iterator_insert(lnk, head, head->next);
}

/**/
_CC_FORCE_INLINE_ void _cc_list_iterator_push_back(_cc_list_iterator_t *head, _cc_list_iterator_t *lnk) {
    _cc_assert(head != nullptr);
    _cc_assert(head->prev != nullptr);
    _cc_list_iterator_insert(lnk, head->prev, head);
}

/**/
_CC_FORCE_INLINE_ void _cc_list_iterator_push(_cc_list_iterator_t *head, _cc_list_iterator_t *lnk) {
    _cc_list_iterator_push_back(head, lnk);
}

/**/
_CC_FORCE_INLINE_ _cc_list_iterator_t *_cc_list_iterator_pop_front(_cc_list_iterator_t *lnk) {
    if (_cc_list_iterator_empty(lnk)) {
        return lnk;
    }

    lnk = lnk->prev;
    _cc_list_iterator_remove(lnk);
    return lnk;
}

/**/
_CC_FORCE_INLINE_ _cc_list_iterator_t *_cc_list_iterator_pop_back(_cc_list_iterator_t *lnk) {
    if (_cc_list_iterator_empty(lnk)) {
        return lnk;
    }

    lnk = lnk->next;
    _cc_list_iterator_remove(lnk);
    return lnk;
}

/**/
_CC_FORCE_INLINE_ _cc_list_iterator_t *_cc_list_iterator_pop(_cc_list_iterator_t *lnk) {
    return _cc_list_iterator_pop_front(lnk);
}

/**/
_CC_FORCE_INLINE_ void _cc_list_iterator_swap(_cc_list_iterator_t *head, _cc_list_iterator_t *lnk) {
    if (!_cc_list_iterator_empty(lnk)) {
        _cc_list_iterator_delete(lnk->prev, lnk->next);
    }
    _cc_list_iterator_push_front(head, lnk);
}

/*
 * @brief join two lists, this is designed for stacks
 *
 * @param head the place to add it in the first list.
 * @param add the new list to add.
 */
_CC_API_PUBLIC(void)
_cc_list_iterator_append(_cc_list_iterator_t *head, _cc_list_iterator_t *add);

/* Return the element at the specified zero-based index
 * where 0 is the head, 1 is the element next to head
 * and so on. Negative integers are used in order to count
 * from the tail, -1 is the last element, -2 the penultimate
 * and so on. If the index is out of range nullptr is returned. */
_CC_API_PUBLIC(_cc_list_iterator_t *)
_cc_list_iterator_index(_cc_list_iterator_t *lnk, long index);

/* the stable insertion sort */
_CC_API_PUBLIC(void)
_cc_list_iterator_sort(_cc_list_iterator_t *lnk,
                       int32_t (*_cmp)(const _cc_list_iterator_t *, const _cc_list_iterator_t *));

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_LIST_ITERATOR_H_INCLUDED_*/
