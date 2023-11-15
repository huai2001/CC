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
#include <cc/event/event.h>
#include <cc/math.h>

/**/
_CC_API_PUBLIC(_cc_event_buffer_t*) _cc_create_event_buffer(void) {
    _cc_event_buffer_t *rw = (_cc_event_buffer_t *)_cc_malloc(sizeof(_cc_event_buffer_t));
    bzero(rw, sizeof(_cc_event_buffer_t));
    rw->r.length = 0;
    rw->w.length = 0;
    rw->w.r = 0;
    rw->w.w = 0;

    rw->w.buf = (byte_t *)_cc_malloc(_CC_IO_BUFFER_SIZE_);
    rw->w.length = _CC_IO_BUFFER_SIZE_;
    
    _cc_spin_lock_init(&(rw->w.wlock));
    return rw;
}

_CC_API_PUBLIC(void) _cc_alloc_event_wbuf(_cc_event_wbuf_t *wbuf, uint16_t length) {
    if (wbuf->w == wbuf->r) {
        if (wbuf->length < length) {
            wbuf->buf = (byte_t *)_cc_realloc(wbuf->buf, length);
            wbuf->length = length;

            if (length > _CC_IO_BUFFER_SIZE_) {
                _cc_logger_warin(_T("socket: The write buffer is too large."));
            }
        }
        wbuf->w = 0;
        wbuf->r = 0;
    } else {
        uint16_t wlen = (wbuf->w - wbuf->r);
        uint16_t len = (wbuf->length - wbuf->w) + wbuf->r;
        if (len < length) {
            wbuf->length = wlen + length;
            wbuf->buf = (byte_t *)_cc_realloc(wbuf->buf, wbuf->length);
            if (wbuf->r > 0) {
                memmove(wbuf->buf, wbuf->buf + wbuf->r, wlen);
                wbuf->w = wlen;
            }
            wbuf->r = 0;

            if (length > _CC_IO_BUFFER_SIZE_) {
                _cc_logger_warin(_T("socket: The write buffer is too large."));
            }
        } else if (wbuf->r > 0 && (wbuf->length - wbuf->w) < length) {
            memmove(wbuf->buf, wbuf->buf + wbuf->r, wlen);
            wbuf->w = wlen;
            wbuf->r = 0;
        }
    }
}

/**/
_CC_API_PUBLIC(bool_t) _cc_copy_event_wbuf(_cc_event_wbuf_t *wbuf, const byte_t *data, uint16_t length) {
    if (_cc_unlikely(length <= 0 || wbuf->buf == NULL)) {
        return false;
    }

    _cc_spin_lock(&wbuf->wlock);
    _cc_alloc_event_wbuf(wbuf, length);
    if (data) {
        memcpy(wbuf->buf + wbuf->w, data, length);
        wbuf->w += length;
    }
    _cc_spin_unlock(&wbuf->wlock);

    return true;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_bind_event_buffer(_cc_event_cycle_t *cycle, _cc_event_buffer_t **rw) {
    *rw = _cc_create_event_buffer();
    if (_cc_unlikely(*rw == NULL)) {
        return false;
    }

    return true;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_unbind_event_buffer(_cc_event_cycle_t *cycle, _cc_event_buffer_t **rw) {
    /**/
    _cc_free((*rw)->w.buf);
    (*rw)->w.buf = NULL;

    _cc_free(*rw);
    *rw = NULL;

    return true;
}

/**/
_CC_API_PUBLIC(int32_t) _cc_event_send(_cc_event_t *e, const byte_t *data, uint16_t length) {
    int32_t off = 0;
    if (_cc_unlikely(e->buffer == NULL)) {
        _cc_logger_error(_T("No write cache was created. e->buffer == NULL"));
        return -1;
    }

    if (_CC_EVENT_WBUF_NO_DATA(e->buffer)) {
        off = _cc_send(e->fd, data, length);
        if (off < 0) {
            return off;
        }

        if (off == length) {
            return length;
        }

        length -= (uint16_t)off;
        data += off;
    }

    /**/
    if (_cc_copy_event_wbuf(&e->buffer->w, data, length)) {
        _cc_event_change_flag(NULL, e, _CC_EVENT_WRITABLE_);
        return off;
    }

    return -1;
}

/**/
_CC_API_PUBLIC(int32_t) _cc_event_sendbuf(_cc_event_t *e) {
    _cc_event_wbuf_t *wbuf;

    int32_t off;
    if (_cc_unlikely(e->buffer == NULL)) {
        _cc_logger_error(_T("No write cache was created. e->buffer == NULL"));
        return -1;
    }

    wbuf = &e->buffer->w;
    if (wbuf->r == wbuf->w) {
        _CC_UNSET_BIT(_CC_EVENT_WRITABLE_, e->flags);
        return 0;
    }

    _cc_spin_lock(&wbuf->wlock);
    off = _cc_send(e->fd, wbuf->buf + wbuf->r, wbuf->w - wbuf->r);
    if (off) {
        wbuf->r += off;

        if (wbuf->r == wbuf->w) {
            _CC_UNSET_BIT(_CC_EVENT_WRITABLE_, e->flags);
        }
    } else if (off < 0) {
        _CC_UNSET_BIT(_CC_EVENT_WRITABLE_, e->flags);
    }
    _cc_spin_unlock(&wbuf->wlock);

    return off;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_event_recv(_cc_event_t *e) {
    int32_t left = 0;
    _cc_event_buffer_t *rw;

    _cc_assert(e->buffer != NULL);
    if (_cc_unlikely(e->buffer == NULL)) {
        _cc_logger_error(_T("No read cache was created. e->buffer == NULL"));
        return false;
    }

    rw = e->buffer;
    if (rw->r.length >= _CC_IO_BUFFER_SIZE_) {
        _cc_logger_error(_T("The space is insufficient.(length: %d >= %d)"), rw->r.length, _CC_IO_BUFFER_SIZE_);
        return false;
    }

#ifdef __CC_ANDROID__
    left = (int32_t)recv(e->fd, (char *)rw->r.buf + rw->r.length, _CC_IO_BUFFER_SIZE_ - rw->r.length, MSG_NOSIGNAL);
#else
    left = (int32_t)recv(e->fd, (char *)rw->r.buf + rw->r.length, _CC_IO_BUFFER_SIZE_ - rw->r.length, 0);
#endif

    if (left < 0) {
        int err = _cc_last_errno();
        if (err == _CC_EINTR_ || err == _CC_EAGAIN_) {
            return true;
        }

        _cc_logger_warin(_T("fd:%d fail to recv (%d): %s"), e->fd, err, _cc_last_error(err));
        return false;
    }

    if (left > 0) {
        rw->r.length += left;
        return true;
    }

    return false;
}
