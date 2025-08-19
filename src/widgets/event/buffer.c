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
#include <libcc/math.h>
#include <libcc/widgets/event.h>

/**/
_CC_API_PUBLIC(_cc_event_buffer_t*) _cc_alloc_event_buffer(void) {
    _cc_event_buffer_t *rw = (_cc_event_buffer_t *)_cc_malloc(sizeof(_cc_event_buffer_t));
    //bzero(rw, sizeof(_cc_event_buffer_t));
    rw->r.length = 0;
    rw->r.limit = _CC_IO_BUFFER_SIZE_;
    rw->w.length = 0;
    rw->w.limit = _CC_IO_BUFFER_SIZE_;

    rw->w.bytes = (byte_t *)_cc_malloc(_CC_IO_BUFFER_SIZE_);
    rw->r.bytes = (byte_t *)_cc_malloc(_CC_IO_BUFFER_SIZE_);

    _cc_lock_init(&(rw->w.lock));
    return rw;
}

/**/
_CC_API_PUBLIC(void) _cc_free_event_buffer(_cc_event_buffer_t *rw) {
    /**/
    _cc_free(rw->w.bytes);
    _cc_free(rw->r.bytes);

    rw->w.bytes = nullptr;
    rw->r.bytes = nullptr;

    _cc_free(rw);
}

_CC_API_PUBLIC(void) _cc_alloc_event_rbuf(_cc_event_rbuf_t *rbuf, int32_t length) {
    rbuf->limit = (int32_t)_cc_aligned_alloc_opt(length, 64);
    rbuf->bytes = (byte_t *)_cc_realloc(rbuf->bytes, rbuf->limit);
}

_CC_API_PUBLIC(void) _cc_alloc_event_wbuf(_cc_event_wbuf_t *wbuf, int32_t length) {
    const int32_t free_length = wbuf->limit - wbuf->length;
    if (free_length < length) {
        wbuf->limit = (int32_t)_cc_aligned_alloc_opt(wbuf->length + length, 64);
        wbuf->bytes = (byte_t *)_cc_realloc(wbuf->bytes, wbuf->limit);
    }
}

/**/
_CC_API_PUBLIC(bool_t) _cc_copy_event_wbuf(_cc_event_wbuf_t *wbuf, const byte_t *data, int32_t length) {
    if (_cc_unlikely(length <= 0 || data == nullptr)) {
        return false;
    }

    _cc_spin_lock(&wbuf->lock);

    _cc_alloc_event_wbuf(wbuf, length);

    memcpy(wbuf->bytes + wbuf->length, data, length);
    wbuf->length += length;

    _cc_unlock(&wbuf->lock);

    return true;
}

/**/
_CC_API_PUBLIC(int32_t) _cc_event_send(_cc_event_t *e, const byte_t *data, int32_t length) {
    int32_t bw = 0;
    _cc_assert(e->buffer != nullptr);

     // nothing queued? See if we can just send this without queueing.
    if (e->buffer->w.length == 0) {
        bw = _cc_send(e->fd, data, length);
        if (bw < 0) {
            return bw;
        }
        // sent the whole thing? We're good to go here.
        if (bw == length) {
            return length;
        }
        // partial write? We'll queue the rest.
        length -= bw;
        data += bw;
    }

    /*queue this up for sending later.*/
    if (_cc_copy_event_wbuf(&e->buffer->w, data, length)) {
        _cc_async_event_t *async = _cc_get_async_event_by_id(e->round);
        _CC_SET_BIT(_CC_EVENT_WRITABLE_, e->flags);
        async->reset(async, e);
        return length;
    }
    return -1;
}

/**/
_CC_API_PUBLIC(int32_t) _cc_event_sendbuf(_cc_event_t *e) {
    int32_t bw;
    _cc_event_wbuf_t *wbuf;
    _cc_assert(e->buffer != nullptr);

    wbuf = &e->buffer->w;
    if (wbuf->length == 0) {
        _CC_UNSET_BIT(_CC_EVENT_WRITABLE_, e->flags);
        return 0;
    }
    
    _cc_spin_lock(&wbuf->lock);
    bw = _cc_send(e->fd, wbuf->bytes, wbuf->length);
    if (bw < 0) {
        _CC_UNSET_BIT(_CC_EVENT_WRITABLE_, e->flags);
    } else if (bw != 0 && bw < wbuf->length) {
        memmove(wbuf->bytes, wbuf->bytes + bw, wbuf->length - bw);
        wbuf->length -= bw;
    } else {
        _CC_UNSET_BIT(_CC_EVENT_WRITABLE_, e->flags);
    }
    _cc_unlock(&wbuf->lock);
    return bw;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_event_recv(_cc_event_t *e) {
    int32_t result = 0;
    _cc_event_buffer_t *rw;
    _cc_assert(e->buffer != nullptr);

    rw = e->buffer;
    if (rw->r.length >= rw->r.limit) {
        _cc_logger_error(_T("The space is insufficient.(length: %d >= %d)"), rw->r.length, rw->r.limit);
        return false;
    }

#ifdef __CC_ANDROID__
    result = (int32_t)recv(e->fd, (char *)rw->r.bytes + rw->r.length, rw->r.limit - rw->r.length, MSG_NOSIGNAL);
#elif defined(__CC_WINDOWS__)
    result = (int32_t)_win_recv(e->fd, rw->r.bytes + rw->r.length, rw->r.limit - rw->r.length);
#else
    result = (int32_t)recv(e->fd, (char *)rw->r.bytes + rw->r.length, rw->r.limit - rw->r.length, 0);
#endif

    if (result < 0) {
        int err = _cc_last_errno();
        if (err == _CC_EINTR_ || err == _CC_EAGAIN_) {
            return true;
        }

        _cc_logger_warin(_T("fd:%d fail to recv (%d): %s"), e->fd, err, _cc_last_error(err));
        return false;
    } else if (result == 0) {
        return false;
    }

    rw->r.length += result;
    return true;
}
