#include <libcc/alloc.h>
#include "sys_iocp.h"

#ifdef _CC_EVENT_USE_IOCP_

void _io_context_init(_cc_async_event_priv_t *priv) {
    _cc_list_iterator_cleanup(&priv->io_active);
    _cc_list_iterator_cleanup(&priv->io_idle);
    priv->frees = 0;
}

void _io_context_quit(_cc_async_event_priv_t *priv) {
    _cc_list_iterator_for_each(it, &priv->io_active, {
        _cc_free(_cc_upcast(it, _io_context_t, lnk));
    });
    _cc_list_iterator_for_each(it, &priv->io_idle, {
        _cc_free(_cc_upcast(it, _io_context_t, lnk));
    });
    _cc_list_iterator_cleanup(&priv->io_active);
    _cc_list_iterator_cleanup(&priv->io_idle);
}

_io_context_t* _io_context_alloc(_cc_async_event_priv_t *priv, _cc_event_t *e) {
    _io_context_t *io_context;
    _cc_list_iterator_t *lnk = _cc_list_iterator_pop(&priv->io_idle);

    if (lnk == &priv->io_idle) {
        io_context = (_io_context_t *)_cc_malloc(sizeof(_io_context_t));
        lnk = &(io_context->lnk);
    } else {
        io_context = _cc_upcast(lnk, _io_context_t, lnk);
        priv->frees--;
    }
	
    bzero(io_context, sizeof(_io_context_t));
    io_context->fd = _CC_INVALID_SOCKET_;
    io_context->e = e;
	io_context->number_of_bytes = 0;
    io_context->ident = e->ident;
    
    _cc_list_iterator_push(&priv->io_active, &(io_context->lnk));

    return io_context;
}

void _io_context_free(_cc_async_event_priv_t *priv, _io_context_t *io_context) {

    if (io_context->fd != _CC_INVALID_SOCKET_) {
        _cc_close_socket(io_context->fd);
        io_context->fd = _CC_INVALID_SOCKET_;
    }
    
    if (priv->frees >= _CC_MAX_CHANGE_EVENTS_) {
        _cc_list_iterator_remove(&io_context->lnk);
        _cc_free(io_context);
        return;
    }

    _cc_list_iterator_swap(&priv->io_idle, &io_context->lnk);
    priv->frees++;
    return;
}

#endif