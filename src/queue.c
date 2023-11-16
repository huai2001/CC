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
#include <cc/queue.h>
#include <cc/atomic.h>

/**/
_CC_API_PUBLIC(void) _cc_queue_sync_push(_cc_queue_iterator_t *head, _cc_queue_iterator_t *lnk) {
    do {
        lnk->next = head->next;
    } while (!_cc_atomic_cas((_cc_atomic_t*)&head->next,((_cc_uint_t)lnk->next), (_cc_uint_t)lnk));
}

/**/
_CC_API_PUBLIC(_cc_queue_iterator_t*) _cc_queue_sync_pop(_cc_queue_iterator_t *head) {
    _cc_queue_iterator_t *lnk;

    do {
        lnk = head->next;
    } while (!_cc_atomic_cas((_cc_atomic_t*)&head->next,  (_cc_uint_t)(head->next), (_cc_uint_t)lnk->next));
    
    return lnk;
}

/* Return the element at the specified zero-based index
 * where 0 is the head, 1 is the element next to head
 * and so on. If the index is out of range NULL is returned. */
_CC_API_PUBLIC(_cc_queue_iterator_t*) _cc_queue_iterator_index(_cc_queue_iterator_t *head, long index) {
    _cc_queue_iterator_t *n;
    if (index < 0) {
        return NULL;
    }

    n = head->next;
    while (index-- && n)
        n = n->next;

    return n;
}
