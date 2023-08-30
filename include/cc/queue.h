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
#ifndef _C_CC_QUEUE_ITERATOR_H_INCLUDED_
#define _C_CC_QUEUE_ITERATOR_H_INCLUDED_

#include "core.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define _cc_queue_iterator_for_each(__CURR, __FIRST, __OP)  \
    do {                                                    \
        _cc_queue_iterator_t* __NEXT = (__FIRST)->next;     \
        _cc_queue_iterator_t* __CURR;                       \
        while (_cc_likely(__NEXT != (__FIRST))) {           \
            __CURR = __NEXT;                                \
            __NEXT = __NEXT->next;                          \
            __OP                                            \
        }                                                   \
    } while (0)

#define _cc_queue_iterator_for(__NEXT, __FIRST) \
    for (__NEXT = (__FIRST)->next; __NEXT != (__FIRST); __NEXT = __NEXT->next)

/**/
typedef struct _cc_queue_iterator _cc_queue_iterator_t;
typedef struct _cc_queue _cc_queue_t;

/**/
struct _cc_queue_iterator {
    _cc_queue_iterator_t *next;
};

_CC_FORCE_INLINE_ _cc_queue_iterator_t *_cc_queue_iterator_first(_cc_queue_iterator_t *lnk) {
    return lnk->next;
}

/**/
_CC_FORCE_INLINE_ void _cc_queue_iterator_cleanup(_cc_queue_iterator_t *lnk) {
    lnk->next = lnk;
}

/**
 * @brief Link empty
 *
 * @param lnk link context
 *
 * @return true if successful or false on error.
 */
_CC_FORCE_INLINE_ bool_t _cc_queue_iterator_empty(_cc_queue_iterator_t *lnk) {
    _cc_assert(lnk != NULL);
    return (lnk->next == lnk || lnk->next == NULL);
}

/**/
_CC_FORCE_INLINE_ void _cc_queue_iterator_push(_cc_queue_iterator_t *head, _cc_queue_iterator_t *lnk) {
    _cc_assert(head != NULL);
    lnk->next = head->next;
    head->next = lnk;
}

/**/
_CC_FORCE_INLINE_ _cc_queue_iterator_t *_cc_queue_iterator_pop(_cc_queue_iterator_t *head) {
    _cc_queue_iterator_t *r;
    if (_cc_queue_iterator_empty(head)) {
        return head;
    }

    r = head->next;
    head->next = r->next;

    _cc_queue_iterator_cleanup(r);
    return r;
}

/**/
_CC_API(void) _cc_queue_sync_push(_cc_queue_iterator_t *head, _cc_queue_iterator_t *lnk);
/**/
_CC_API(_cc_queue_iterator_t*) _cc_queue_sync_pop(_cc_queue_iterator_t *head);
/* Return the element at the specified zero-based index
 * where 0 is the head, 1 is the element next to head
 * and so on. If the index is out of range NULL is returned. */
_CC_API(_cc_queue_iterator_t *) _cc_queue_iterator_index(_cc_queue_iterator_t *head, long index);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#endif /*_C_CC_QUEUE_ITERATOR_H_INCLUDED_*/
