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
#include <libcc/widgets/widgets.h>
#include <libcc.h>
#include <locale.h>
#include <stdio.h>
/*
https://www.ietf.org/rfc/rfc1928.txt
+----+----------+----------+
|VER | NMETHODS | METHODS  |
+----+----------+----------+
| 1  |    1     | 1 to 255 |
+----+----------+----------+
METHOD:
 0x00 不需要认证（常用）
 0x01 GSSAPI认证
 0x02 账号密码认证（常用）
 0x03 - 0x7F IANA分配
 0x80 - 0xFE 私有方法保留
 0xFF 无支持的认证方法
*/

typedef struct _socks5 {
    byte_t status;
    byte_t method;
    _cc_event_t *e;
    _cc_union_sockaddr_t addr;

    uint16_t port;

} _cc_socks5_t;

static bool_t _socks5_event_callback2(_cc_event_cycle_t *cycle, _cc_event_t *e, const uint16_t which) {
    if (which & _CC_EVENT_CONNECTED_) {
        _cc_logger_debug(_T("%d connect to server."), e->fd);
        return true;
    }

    if (which & _CC_EVENT_DISCONNECT_) {
        int32_t results = _cc_last_errno();
        _cc_logger_error(_T("Socket(%d) disconnect:%d, %s"), e->fd, results, _cc_last_error(results));
        return false;
    }

    if (which & _CC_EVENT_READABLE_) {
        byte_t buf[1024 * 16];
        _cc_event_t *ow = (_cc_event_t *)e->args;
        int32_t length;

        length = _cc_recv(e->fd, buf, _cc_countof(buf));
        if (length <= 0) {
            _cc_logger_debug(_T("%d close to client."), e->fd);
            return false;
        }

        if (ow && ow->fd != _CC_INVALID_SOCKET_ && (ow->flags & _CC_EVENT_DISCONNECT_) == 0) {
            return _cc_event_send(ow, buf, length);
        }
        return false;
    }

    if (which & _CC_EVENT_WRITABLE_) {
        if (which & _CC_EVENT_WRITABLE_) {
            int32_t off = _cc_event_sendbuf(e);
            if (off < 0) {
                return false;
            }
            _cc_sleep(1);

            if ((e->flags & _CC_EVENT_WRITABLE_) == 0 && e->flags & _CC_EVENT_DISCONNECT_) {
                return false;
            }
        }
    }

    if (which & _CC_EVENT_TIMEOUT_) {
        _cc_logger_debug(_T("TCP timeout %d"), e->fd);
        return false;
    }
    return true;
}

void _socks_send_res(_cc_event_t *e, byte_t a, byte_t b) {
    byte_t buf[2];
    buf[0] = a;
    buf[1] = b;

    _cc_event_send(e, buf, 2);
}

bool_t ShakeHands(byte_t *m, _cc_socks5_t *socks, _cc_event_t *e) {
    if (*m != 0x05) {
        return false;
    }

    socks->method = *(m + 2);
    if (socks->method == 0x02) {
        socks->status = 1;
    } else {
        socks->status = 3;
    }
    _socks_send_res(e, 0x05, socks->method);
    return true;
}

bool_t ValidateIdentity(byte_t *m, _cc_socks5_t *socks, _cc_event_t *e) {
    byte_t user_len;
    byte_t pass_len;
    if (*m != 0x05) {
        return false;
    }

    user_len = *(m + 1);
    pass_len = *(m + user_len + 1);

    _socks_send_res(e, 0x01, 0x00);
    socks->status = 3;
    return true;
}

bool_t ProtocolRequest(byte_t *m, _cc_socks5_t *socks, _cc_event_t *e) {
    byte_t atype;
    byte_t rep = 0x07;
    uint16_t port;
    byte_t buf[128];
    byte_t domain_len;
    uint32_t ip;

    if (*m != 0x05) {
        return false;
    }
    atype = *(m + 3);

    switch (atype) {
    case _CC_SOCKS5_ADDRESS_TYPE_IPV4_:
        // socks->port = (uint16_t)((m + 7));
        memcpy(buf, m + 4, 4);
        memcpy(&socks->port, ((m + 7)), sizeof(uint16_t));
        break;
    case _CC_SOCKS5_ADDRESS_TYPE_DOMAIN_: {
        domain_len = *(m + 4);
        // socks->port = (uint16_t)((m + 5 + domain_len));
        memcpy(buf, m + 5, domain_len);
        memcpy(&socks->port, ((m + 5 + domain_len)), sizeof(uint16_t));
        buf[domain_len] = 0;

    } break;
    case _CC_SOCKS5_ADDRESS_TYPE_IPV6_: {
        // socks->port = (uint16_t)(*(m + 3 + 16));
        memcpy(buf, m + 3, 16);
        memcpy(&socks->port, ((m + 19)), sizeof(uint16_t));
        buf[16] = 0;

    } break;
    default:
        rep = 0x08;
        break;
    }

    if (rep == 0x07) {
        socks->port = _cc_swap16(socks->port);
        _cc_logger_debug(_T("IP:%s, Port:%d."), buf, socks->port);
        if (atype == _CC_SOCKS5_ADDRESS_TYPE_IPV6_) {
            _cc_inet_ipv6_addr(&socks->addr_in6, (const tchar_t *)buf, socks->port);
        } else {
            _cc_inet_ipv4_addr(&socks->addr_in, (const tchar_t *)buf, socks->port);
        }
        rep = 0x00;
    }

    buf[0] = 0x05;
    buf[1] = rep;
    buf[2] = 0x00;
    buf[3] = 0x01;

    port = htons(8088);
    ip = inet_addr("127.0.0.1");

    memcpy(&buf[4], &ip, 4);
    memcpy(&buf[8], &port, 2);

    socks->e = _cc_tcp_connect(_cc_get_event_cycle(), _CC_EVENT_CONNECT_ | _CC_EVENT_TIMEOUT_ | _CC_EVENT_BUFFER_,
                               (_cc_sockaddr_t *)&socks->addr, 60000, _socks5_event_callback2, e);

    socks->status = 4;
    _cc_event_send(e, buf, 10);
    return true;
}

static bool_t _socks5_event_callback(_cc_event_cycle_t *cycle, _cc_event_t *e, const uint16_t which) {
    if (which & _CC_EVENT_ACCEPT_) {
        _cc_socket_t fd;
        _cc_event_t *event;
        _cc_socks5_t *socks5;
        struct sockaddr_in remote_addr = {0};
        _cc_socklen_t remote_addr_len = sizeof(struct sockaddr_in);
        _cc_event_cycle_t *cycle_new = _cc_get_event_cycle();

        fd = _cc_event_accept(cycle, e, &remote_addr, &remote_addr_len);
        if (fd == _CC_INVALID_SOCKET_) {
            _cc_logger_debug(_T("thread %d accept fail %s."), _cc_get_thread_id(nullptr),
                             _cc_last_error(_cc_last_errno()));
            return true;
        }

        event = _cc_event_alloc(cycle_new, _CC_EVENT_TIMEOUT_ | _CC_EVENT_READABLE_ | _CC_EVENT_BUFFER_);
        if (event == nullptr) {
            _cc_close_socket(fd);
            return true;
        }

        _cc_set_socket_nonblock(fd, 1);

        socks5 = _cc_malloc(sizeof(_cc_socks5_t));
        socks5->status = 0;
        socks5->e = event;

        event->fd = fd;
        event->callback = e->callback;
        event->args = socks5;
        event->timeout = e->timeout;

        if (cycle_new->attach(cycle_new, event) == false) {
            _cc_logger_debug(_T("thread %d add socket (%d) event fial."), _cc_get_thread_id(nullptr), fd);
            _cc_free_event(cycle_new, event);
            return true;
        }
        /*
        {
            struct sockaddr_in* remote_ip = (struct sockaddr_in*)&remote_addr;
            byte_t *ip_addr = (byte_t *)&remote_ip->sin_addr.s_addr;
            _cc_logger_debug(_T("TCP accept [%d,%d,%d,%d] fd:%d"), ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3], fd);
        }*/

        return true;
    }

    if (which & _CC_EVENT_DISCONNECT_) {
        _cc_logger_debug(_T("%d client disconnect."), e->fd);
        return false;
    }

    if (which & _CC_EVENT_READABLE_) {
        _cc_event_buffer_t *rw = e->buffer;
        _cc_socks5_t *socks = (_cc_socks5_t *)e->args;
        if (!_cc_event_recv(e)) {
            _cc_logger_debug(_T("%d client close."), e->fd);
            return false;
        }

        if (socks->status == 0) {
            rw->r.length = 0;
            return ShakeHands(rw->r.bytes, socks, e);
        }

        if (socks->status == 1) {
            rw->r.length = 0;
            return ValidateIdentity(rw->r.bytes, socks, e);
        }

        if (socks->status == 3) {
            rw->r.length = 0;
            return ProtocolRequest(rw->r.bytes, socks, e);
        }

        if (socks->status == 4) {
            _cc_event_t *ow = socks->e;
            // rw->r.bytes[rw->r.length] = 0;
            // printf("%s\n", rw->r.bytes);

            if (ow && ow->fd != _CC_INVALID_SOCKET_ && (ow->flags & _CC_EVENT_DISCONNECT_) == 0) {
                _cc_event_send(ow, rw->r.bytes, rw->r.length);
                rw->r.length = 0;
            } else {
                return false;
            }
        }

        _cc_sleep(1);
        return true;
    }

    if (which & _CC_EVENT_WRITABLE_) {
        int32_t off = _cc_event_sendbuf(e);
        if (off < 0) {
            return false;
        }

        _cc_sleep(1);

        if ((e->flags & _CC_EVENT_WRITABLE_) == 0 && e->flags & _CC_EVENT_DISCONNECT_) {
            return false;
        }
    }

    if (which & _CC_EVENT_TIMEOUT_) {
        _cc_logger_debug(_T("%d client timeout."), e->fd);
        return false;
    }

    return true;
}

void _cc_socks5_starting(uint16_t port) {
    struct sockaddr_in sa;
    _cc_event_cycle_t *cycle = _cc_get_event_cycle();
    _cc_evnet_t *e = _cc_event_alloc(cycle, _CC_EVENT_ACCEPT_);
    if (e == nullptr) {
        return;
    }
    e->timeout = 300000;
    e->callback = _socks5_event_callback;

    _cc_inet_ipv4_addr(&sa, nullptr, port);
    if (!_cc_tcp_listen(cycle, e, (_cc_sockaddr_t *)&sa, sizeof(struct sockaddr_in))) {
        _cc_free_event(cycle, e);
    }
}

int main(int argc, char *argv[]) {
    _cc_event_loop(0, nullptr);

    _cc_socks5_starting(8088);
    while (getchar() != 'q') {
        _cc_sleep(100);
    }
    _cc_quit_event_loop();
    return 0;
}
