#ifndef _C_CC_QUEUE_ITERATOR_H_INCLUDED_
#define _C_CC_QUEUE_ITERATOR_H_INCLUDED_

#include "os.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define _cc_queue_iterator_for_each(__CURR, __FIRST, __OP)  \
    do {                                                    \
        _cc_queue_iterator_t* __NEXT = (__FIRST)->next;     \
        _cc_queue_iterator_t* __CURR;                       \
        while (__NEXT != (__FIRST)) {           \
            __CURR = __NEXT;                                \
            __NEXT = __NEXT->next;                          \
            __OP                                            \
        }                                                   \
    } while (0)

#define _cc_queue_iterator_for(__NEXT, __FIRST) \
    for (__NEXT = (__FIRST)->next; __NEXT != (__FIRST); __NEXT = __NEXT->next)

/**/
typedef struct _cc_queue_iterator _cc_queue_iterator_t;

/**/
struct _cc_queue_iterator {
    _cc_queue_iterator_t *next;
};

/**/
_CC_FORCE_INLINE_ _cc_queue_iterator_t *_cc_queue_iterator_first(_cc_queue_iterator_t *lnk) {
    return lnk->next;
}

/**/
_CC_FORCE_INLINE_ void _cc_queue_iterator_cleanup(_cc_queue_iterator_t *lnk) {
    lnk->next = lnk;
}

/**/
_CC_FORCE_INLINE_ bool_t _cc_queue_iterator_empty(_cc_queue_iterator_t *lnk) {
    _cc_assert(lnk != nullptr);
    return (lnk->next == lnk || lnk->next == nullptr);
}

/**/
_CC_FORCE_INLINE_ void _cc_queue_iterator_push(_cc_queue_iterator_t *head, _cc_queue_iterator_t *lnk) {
    _cc_assert(head != nullptr);
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
_CC_API_PUBLIC(void) _cc_queue_sync_push(_cc_queue_iterator_t *head, _cc_queue_iterator_t *lnk);
/**/
_CC_API_PUBLIC(_cc_queue_iterator_t*) _cc_queue_sync_pop(_cc_queue_iterator_t *head);

/* Return the element at the specified zero-based index
 * where 0 is the head, 1 is the element next to head
 * and so on. If the index is out of range nullptr is returned. */
_CC_API_PUBLIC(_cc_queue_iterator_t *) _cc_queue_iterator_index(_cc_queue_iterator_t *head, long index);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#endif /*_C_CC_QUEUE_ITERATOR_H_INCLUDED_*/
