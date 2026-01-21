#include <libcc/alloc.h>
#include <libcc/math.h>
#include "event.c.h"

_CC_API_PUBLIC(_cc_io_buffer_t*) _cc_alloc_io_buffer(int32_t limit) {
    _cc_io_buffer_t *io = (_cc_io_buffer_t *)_cc_malloc(sizeof(_cc_io_buffer_t));
    io->r.limit = limit;
    io->r.off = 0;
    io->r.bytes = (byte_t*)_cc_calloc(limit,sizeof(byte_t));

    io->w.limit = limit;
    io->w.off = 0;
    io->w.bytes = (byte_t*)_cc_calloc(limit,sizeof(byte_t));

#ifdef _CC_USE_OPENSSL_
    io->ssl = NULL;
#endif
    io->lock_of_writable = _cc_alloc_mutex();
    return io;
}

/**/
_CC_API_PUBLIC(void) _cc_realloc_read_buffer(_cc_io_buffer_t *io, int32_t limit) {
    io->r.limit = limit;
    io->r.bytes = (byte_t*)_cc_realloc(io->r.bytes,limit * sizeof(byte_t));
    if (io->r.off > limit) {
        io->r.off = 0;
    }
}

/**/
_CC_API_PUBLIC(void) _cc_realloc_write_buffer(_cc_io_buffer_t *io, int32_t limit) {
    io->w.limit = limit;
    io->w.bytes = (byte_t*)_cc_realloc(io->w.bytes,limit * sizeof(byte_t));
    if (io->w.off > limit) {
        io->w.off = 0;
    }
}

/**/
_CC_API_PUBLIC(void) _cc_free_io_buffer(_cc_io_buffer_t *io) {
    /**/
    _cc_assert(io != NULL);
    _cc_assert(io->r.bytes != NULL);
    _cc_assert(io->w.bytes != NULL);

    _cc_free(io->w.bytes);
    _cc_free(io->r.bytes);

    if (io->lock_of_writable) {
        _cc_free_mutex(io->lock_of_writable);
    }

#ifdef _CC_USE_OPENSSL_
    if (io->ssl) {
        _SSL_free(io->ssl);
        io->ssl = NULL;
    }
#endif
    _cc_free(io);
}

_CC_API_PRIVATE(int32_t) _send(_cc_event_t *e, _cc_io_buffer_t *data, const byte_t *bytes, int32_t length) {
#ifdef _CC_USE_OPENSSL_
    if (data->ssl) {
        return _SSL_send(data->ssl, bytes, length);
    }
#endif
    return _cc_send(e->fd, bytes, length);
}

/**/
_CC_API_PUBLIC(int32_t) _cc_io_buffer_send(_cc_event_t *e, _cc_io_buffer_t *data, const byte_t *bytes, int32_t length) {
    int32_t off = 0, required_length;
   
    _cc_mutex_lock(data->lock_of_writable);
    if (data->w.off == 0) {
        // nothing queued? See if we can just send this without queueing.
        off = _send(e, data, bytes, length);
        if (length == off || off < 0) {
            _cc_mutex_unlock(data->lock_of_writable);
            return off;
        }
        bytes += off;
        length -= off;
    }

    required_length = length + data->w.off;
    /*queue this up for sending later.*/
    if (required_length >= data->w.limit) {
        data->w.limit = required_length + (int32_t)(data->w.limit * 0.72f);
        // uhoh, overflowed! That's a lot of memory!!
        if (data->w.limit <= 0) {
            _cc_abort(_T("uhoh, overflowed! That's a lot of memory!!"));
        }
        data->w.bytes = (byte_t*)_cc_realloc(data->w.bytes, data->w.limit);
    }

    memcpy(data->w.bytes + data->w.off, bytes + off, length);
    data->w.off += length;

    _CC_SET_BIT(_CC_EVENT_WRITABLE_, e->flags);
    _cc_mutex_unlock(data->lock_of_writable);

    return off;
}

/**/
_CC_API_PUBLIC(int32_t) _cc_io_buffer_flush(_cc_event_t *e, _cc_io_buffer_t *data) {
    int32_t off;
    _cc_assert(data != 0);
    if (data->w.off == 0) {
        _CC_UNSET_BIT(_CC_EVENT_WRITABLE_, e->flags);
        return 0;
    }

    _cc_mutex_lock(data->lock_of_writable);
    off = _send(e, data, data->w.bytes, data->w.off);
    if (off == data->w.off || off < 0) {
        data->w.off = 0;
        _CC_UNSET_BIT(_CC_EVENT_WRITABLE_, e->flags);
    } else if (off > 0) {
        data->w.off -= off;
        memmove(data->w.bytes, data->w.bytes + off, data->w.off);
        _CC_SET_BIT(_CC_EVENT_WRITABLE_, e->flags);
    }
    _cc_mutex_unlock(data->lock_of_writable);
    return off;
}

/**/
_CC_API_PUBLIC(int32_t) _cc_io_buffer_read(_cc_event_t *e, _cc_io_buffer_t *data) {
    int32_t off = 0;

#ifdef _CC_USE_OPENSSL_
    if (data->ssl) {
        off = _SSL_read(data->ssl, data->r.bytes + data->r.off, data->r.limit - data->r.off);
        if (off > 0) {
            data->r.off += off;
        }
        return off;
    }
#endif

#ifdef __CC_ANDROID__
    off = (int32_t)recv(e->fd, (char *)data->r.bytes + data->r.off, data->r.limit - data->r.off, MSG_NOSIGNAL);
#else
    off = (int32_t)recv(e->fd, (char *)data->r.bytes + data->r.off, data->r.limit - data->r.off, 0);
#endif
    if (off == 0) {
        //End of stream
        return -1;
    } else if (off < 0) {
        int er = _cc_last_errno();
        if (er == _CC_EINTR_ || er == _CC_EAGAIN_) {
            return 0;
        }
        _cc_logger_warin("fd:%d fail to recv (%d): %s", e->fd, er, _cc_last_error(er));
    }
    
    data->r.off += off;
    return off;
}

