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
#include <cc/alloc.h>
#include "sys_socket_c.h"

void _cc_iocp_overlapped_init(_cc_event_cycle_priv_t *ed) {
    _cc_list_iterator_cleanup(&ed->overlapped_active);
    _cc_list_iterator_cleanup(&ed->overlapped_idle);
    _cc_spin_lock_init(&ed->lock);
    ed->idle_count = 0;
}

void _cc_iocp_overlapped_quit(_cc_event_cycle_priv_t *ed) {
    _cc_spin_lock(&ed->lock);
    _cc_list_iterator_for_each(val, &ed->overlapped_active, {
        _iocp_overlapped_t *o = _cc_upcast(val, _iocp_overlapped_t, lnk);
        _cc_free(o);
    });
    _cc_list_iterator_for_each(val, &ed->overlapped_idle, {
        _iocp_overlapped_t *o = _cc_upcast(val, _iocp_overlapped_t, lnk);
        _cc_free(o);
    });
    _cc_list_iterator_cleanup(&ed->overlapped_active);
    _cc_list_iterator_cleanup(&ed->overlapped_idle);
    _cc_spin_unlock(&ed->lock);
}

bool_t _cc_iocp_overlapped_alloc(_cc_event_cycle_priv_t *ed, _iocp_overlapped_t **iocp_overlapped) {
    _cc_list_iterator_t *lnk;

    _cc_spin_lock(&ed->lock);
    lnk = _cc_list_iterator_pop(&ed->overlapped_idle);
    _cc_spin_unlock(&ed->lock);

    if (lnk == &ed->overlapped_idle) {
        *iocp_overlapped = (_iocp_overlapped_t *)_cc_malloc(sizeof(_iocp_overlapped_t));
        lnk = &(*iocp_overlapped)->lnk;
        bzero(*iocp_overlapped, sizeof(_iocp_overlapped_t));
    } else {
        *iocp_overlapped = _cc_upcast(lnk, _iocp_overlapped_t, lnk);
        ed->idle_count--;
    }

    _cc_spin_lock(&ed->lock);
    _cc_list_iterator_push(&ed->overlapped_active, &(*iocp_overlapped)->lnk);
    _cc_spin_unlock(&ed->lock);

    return true;
}

bool_t _cc_iocp_overlapped_free(_cc_event_cycle_priv_t *ed, _iocp_overlapped_t *iocp_overlapped) {
    if (ed->idle_count >= 64) {
        _cc_spin_lock(&ed->lock);
        _cc_list_iterator_remove(&iocp_overlapped->lnk);
        _cc_spin_unlock(&ed->lock);
        _cc_free(iocp_overlapped);
        return true;
    }
    _cc_spin_lock(&ed->lock);
    _cc_list_iterator_swap(&ed->overlapped_idle, &iocp_overlapped->lnk);
    ed->idle_count++;
    _cc_spin_unlock(&ed->lock);
    return true;
}