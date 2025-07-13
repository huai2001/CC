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
#include <libcc/list.h>

/* Return the element at the specified zero-based index
 * where 0 is the head, 1 is the element next to head
 * and so on. Negative integers are used in order to count
 * from the tail, -1 is the last element, -2 the penultimate
 * and so on. If the index is out of range nullptr is returned. */
_CC_API_PUBLIC(_cc_list_iterator_t *) _cc_list_iterator_index(_cc_list_iterator_t *head, long index) {
    _cc_list_iterator_t *n;
    if (index < 0) {
        index = (-index) - 1;
        n = head->prev;
        while (index-- && n)
            n = n->prev;
    } else {
        n = head->next;
        while (index-- && n)
            n = n->next;
    }
    return n;
}

_CC_API_PRIVATE(void) __list_append(_cc_list_iterator_t *list, _cc_list_iterator_t *prev, _cc_list_iterator_t *next) {
    _cc_list_iterator_t *first = list->next;
    _cc_list_iterator_t *last = list->prev;

    first->prev = prev;
    prev->next = first;

    last->next = next;
    next->prev = last;
}
/**
 * _cc_list_iterator_append - join two lists, this is designed for stacks
 * @lnk: the place to add it in the first list.
 * @add: the new list to add.
 */
_CC_API_PUBLIC(void) _cc_list_iterator_append(_cc_list_iterator_t *head, _cc_list_iterator_t *add) {
    if (_cc_list_iterator_empty(add)) {
        return;
    }
    //
    __list_append(add, head, head->next);
    //__list_append(add, head->prev, head);
}

/* the stable insertion sort */
_CC_API_PUBLIC(void)
_cc_list_iterator_sort(_cc_list_iterator_t *lnk,
                       int32_t (*_cmp)(const _cc_list_iterator_t *, const _cc_list_iterator_t *)) {
    _cc_list_iterator_t *q, *prev, *next;
    q = lnk->next;

    if (q == lnk->prev) {
        return;
    }

    /**/
    for (q = q->next; q != lnk; q = next) {
        prev = q->prev;
        next = q->next;

        _cc_list_iterator_remove(q);

        do {
            if (_cmp(prev, q) <= 0) {
                break;
            }
            prev = prev->prev;
        } while (prev != lnk);

        _cc_list_iterator_insert(q, prev, prev->next);
    }
}