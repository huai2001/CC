#include <libcc/atomic.h>
#include <libcc/queue.h>

/**/
_CC_API_PUBLIC(void) _cc_queue_sync_push(_cc_queue_t *head, _cc_queue_t *lnk) {
    do {
        lnk->next = head->next;
    } while (!_cc_atomic64_cas((_cc_atomic64_t *)(uintptr_t)&head->next, (uintptr_t)lnk->next, (uintptr_t)lnk));
}

/**/
_CC_API_PUBLIC(_cc_queue_t *) _cc_queue_sync_pop(_cc_queue_t *head) {
    _cc_queue_t *lnk;
    do {
        lnk = head->next;
        if (head == lnk) {
            return head;
        }
    } while (!_cc_atomic64_cas((_cc_atomic64_t *)(uintptr_t)&head->next, (uintptr_t)lnk, (uintptr_t)lnk->next));
    return lnk;
}

/* Return the element at the specified zero-based index
 * where 0 is the head, 1 is the element next to head
 * and so on. If the index is out of range NULL is returned. */
_CC_API_PUBLIC(_cc_queue_t *) _cc_queue_index(_cc_queue_t *head, long index) {
    _cc_queue_t *n;
    if (index < 0) {
        return NULL;
    }

    n = head->next;
    while (index-- && n)
        n = n->next;

    return n;
}
