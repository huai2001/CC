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

#include <cc/widgets/socks5.h>
#include <libcc.h>
#include <cc/alloc.h>

typedef struct _socks5 {
    byte_t status;
    _cc_event_t *e;
}_cc_socks5_t

_CC_API_PRIVATE(bool_t) _socks5_event_callback(_cc_event_cycle_t *cycle, _cc_event_t *e, const uint16_t events) {
    if (events & _CC_EVENT_ACCEPT_) {
        _cc_socket_t fd;
        _cc_event_t *event;
        _cc_sockaddr_t remote_addr = {0};
        _cc_socklen_t remote_addr_len = sizeof(_cc_sockaddr_t);
        _cc_event_cycle_t *cycle_new = _cc_get_event_cycle();
        fd = _cc_event_accept(cycle, e, &remote_addr, &remote_addr_len);
        if (fd == _CC_INVALID_SOCKET_) {
            _cc_logger_debug(_T("thread %d accept fail %s."), _cc_get_thread_id(NULL), _cc_last_error(_cc_last_errno()));
            return true;
        }
        
        event = _cc_alloc_event(cycle_new, _CC_EVENT_TIMEOUT_ | _CC_EVENT_READABLE_ | _CC_EVENT_BUFFER_);
        if (event == NULL) {
            _cc_close_socket(fd);
            return true;
        }

        _cc_set_socket_nonblock(fd, 1);
        
        event->fd = fd;
        event->callback = e->callback;
        event->timeout = e->timeout;
        _cc_set_socket_nonblock(fd, 1);

        if (cycle_new->driver.attach(cycle_new, event) == false) {
            _cc_logger_debug(_T("thread %d add socket (%d) event fial."), _cc_get_thread_id(NULL), fd);
            _cc_free_event(cycle_new, event);
            return true;
        }

        /*
        {
            struct sockaddr_in* remote_ip = (struct sockaddr_in*)&remote_addr;
            byte_t *ip_addr = (byte_t *)&remote_ip->sin_addr.s_addr;
            _cc_logger_debug(_T("TCP accept [%d,%d,%d,%d] fd:%d"), ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3], fd);
        }*/
        _cc_atomic32_inc(&monitoring.live);
        _cc_atomic32_inc(&monitoring.total);

        return true;
    }
    
    if (events & _CC_EVENT_DISCONNECT_) {
        //_cc_logger_debug(_T("%d disconnect to client."), e->fd);
        _cc_atomic32_dec(&monitoring.live);
        return false;
    }
    
    if (events & _CC_EVENT_READABLE_) {
        /*_cc_event_buffer_t *rw = e->buffer;
        if (e->buffer == NULL) {
            _cc_bind_event_buffer(cycle, &e->buffer);
            rw = e->buffer;
        }
        if (!_cc_event_recv(e)) {
            //_cc_logger_debug(_T("%d close to client."), e->fd);
            _cc_atomic32_dec(&monitoring.live);
            return false;
        }
        if (rw->r.length > 0) {
            _cc_atomic64_add(&monitoring.revice_size, rw->r.length);
            send_data(cycle, e, rw->r.buf, rw->r.length);
        }

        //_cc_sleep(1);
        rw->r.length = 0;*/
        byte_t buf[1024 * 16];
        int32_t length;

        length = _cc_recv(e->fd, buf, _cc_countof(buf));
        if (length <= 0) {
            //_cc_logger_debug(_T("%d close to client."), e->fd);
            _cc_atomic32_dec(&monitoring.live);
            return false;
        }

        buf[length] = 0;
        _cc_logger_debug("-----\n(%d)\n----\n%s\n",e->fd, buf);

        _cc_atomic64_add(&monitoring.revice_size, length);
        //send_data(cycle, e, buf, length);
        _cc_sleep(1);
        return true;
    }
    
    if (events & _CC_EVENT_WRITABLE_) {
        int32_t off = _cc_event_sendbuf(e);
        if (off < 0) {
            return false;
        }

        _cc_atomic64_add(&monitoring.send_size, off);
        _cc_sleep(1);

        if ((e->flags & _CC_EVENT_WRITABLE_) == 0 && e->flags & _CC_EVENT_DISCONNECT_) {
            return false;
        }
    }
    
    if (events & _CC_EVENT_TIMEOUT_) {
        if (_cc_send(e->fd, (byte_t*)_T("ping"), 5) < 0) {
            _cc_atomic32_dec(&monitoring.live);
            return false;
        }
    }
    
    return true;
}

void _cc_socks5_starting(uint16_t port) {
    struct sockaddr_in sa;

    _cc_event_cycle_t *cycle = _cc_get_event_cycle();
    _cc_evnet_t *e = _cc_alloc_event(cycle, _CC_EVENT_ACCEPT_);
    if (e == NULL) {
        return;
    }
    e->timeout = 300000;
    e->callback = _socks5_event_callback;

    _cc_inet_ipv4_addr(&sa, NULL, port);
    if (!_cc_tcp_listen(cycle, e, (_cc_sockaddr_t *)&sa, sizeof(struct sockaddr_in))) {
        _cc_free_event(cycle, e);
    }
}