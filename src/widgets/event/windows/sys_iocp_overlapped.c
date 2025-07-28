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
#include <libcc/alloc.h>
#include "sys_socket_c.h"

_CC_API_PUBLIC(void) _iocp_overlapped_init(_cc_event_cycle_priv_t *priv) {
    _cc_list_iterator_cleanup(&priv->overlapped_active);
    _cc_list_iterator_cleanup(&priv->overlapped_idle);
    _cc_lock_init(&priv->lock);
    priv->idle_count = 0;
}

_CC_API_PUBLIC(void) _iocp_overlapped_quit(_cc_event_cycle_priv_t *priv) {
    _cc_spin_lock(&priv->lock);
    _cc_list_iterator_for_each(it, &priv->overlapped_active, {
        _cc_free(_cc_upcast(it, _iocp_overlapped_t, lnk));
    });
    _cc_list_iterator_for_each(it, &priv->overlapped_idle, {
        _cc_free(_cc_upcast(it, _iocp_overlapped_t, lnk));
    });
    _cc_list_iterator_cleanup(&priv->overlapped_active);
    _cc_list_iterator_cleanup(&priv->overlapped_idle);
    _cc_unlock(&priv->lock);
}

_CC_API_PUBLIC(_iocp_overlapped_t*) _iocp_overlapped_alloc(_cc_event_cycle_priv_t *priv, _cc_event_t *e) {
    _iocp_overlapped_t *iocp_overlapped;
    _cc_list_iterator_t *lnk;

    _cc_spin_lock(&priv->lock);
    lnk = _cc_list_iterator_pop(&priv->overlapped_idle);
    _cc_unlock(&priv->lock);

    if (lnk == &priv->overlapped_idle) {
        iocp_overlapped = (_iocp_overlapped_t *)_cc_malloc(sizeof(_iocp_overlapped_t));
        bzero(iocp_overlapped, sizeof(_iocp_overlapped_t));
        lnk = &(iocp_overlapped->lnk);
    } else {
        iocp_overlapped = _cc_upcast(lnk, _iocp_overlapped_t, lnk);
        priv->idle_count--;
    }

    iocp_overlapped->fd = _CC_INVALID_SOCKET_;
    iocp_overlapped->e = e;
    iocp_overlapped->round = e->round;
    
    _cc_spin_lock(&priv->lock);
    _cc_list_iterator_push(&priv->overlapped_active, &(iocp_overlapped->lnk));
    _cc_unlock(&priv->lock);

    return iocp_overlapped;
}

_CC_API_PUBLIC(void) _iocp_overlapped_free(_cc_event_cycle_priv_t *priv, _iocp_overlapped_t *iocp_overlapped) {
    if (priv->idle_count >= 64) {
        _cc_spin_lock(&priv->lock);
        _cc_list_iterator_remove(&iocp_overlapped->lnk);
        _cc_unlock(&priv->lock);
        _cc_free(iocp_overlapped);
        return;
    }
    _cc_spin_lock(&priv->lock);
    _cc_list_iterator_swap(&priv->overlapped_idle, &iocp_overlapped->lnk);
    priv->idle_count++;
    _cc_unlock(&priv->lock);
    return;
}