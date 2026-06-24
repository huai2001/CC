#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <libcc/thread.h>
#include <libcc/event.h>
#include <libcc/timeout.h>
static int c = 0;
static uint16_t port = 5500;

void _do_accept(_cc_async_event_t *async, _cc_event_t *e) {
    _cc_socket_t fd;
    _cc_event_t *event;
    struct sockaddr_in remote_addr = {0};
    _cc_socklen_t remote_addr_len = sizeof(struct sockaddr_in);
    _cc_async_event_t *async2 = _cc_get_async_event();

    fd = async->accept(async, e, (_cc_sockaddr_t *)&remote_addr, &remote_addr_len);
    if (fd == _CC_INVALID_SOCKET_) {
        printf("thread %d accept fail %s.", _cc_get_thread_id(NULL),
                         _cc_last_error(_cc_last_errno()));
        return ;
    }

    event = _cc_alloc_event(async2, _CC_EVENT_TIMEOUT_ | _CC_EVENT_READABLE_);
    if (event == NULL) {
        _cc_close_socket(fd);
        return ;
    }

    _cc_set_socket_nonblock(fd, 1);

    event->fd = fd;
    event->callback = e->callback;
    event->timeout = e->timeout;

    if (async2->attach(async2, event) == false) {
        printf("thread %d add socket (%d) event fial.", _cc_get_thread_id(NULL), fd);
        _cc_free_event(async2, event);
    }
    {
        struct sockaddr_in* remote_ip = (struct sockaddr_in*)&remote_addr;
        byte_t *ip_addr = (byte_t *)&remote_ip->sin_addr.s_addr;
        printf("TCP accept [%d,%d,%d,%d] fd:%d", ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3], fd);
    }
}

static bool_t _do_event_handler(_cc_async_event_t *async, _cc_event_t *e, const uint32_t which) {
    if (which & _CC_EVENT_ACCEPT_) {
		printf("%d accept.", e->ident);
        _do_accept(async,e);
        return true;
    }

	if (which & _CC_EVENT_CLOSED_) {
        printf("%d disconnect.", e->ident);
        return false;
    }

    if (which & _CC_EVENT_READABLE_) {
        byte_t buf[_CC_IO_BUFFER_SIZE_];
        int off = _cc_recv(e->fd, buf, _cc_countof(buf));
        if (off < 0) {
            printf("%d recv fail.", e->ident);
            return false;
        } else if (off == 0) {
            printf("%d client close.", e->ident);
            return false;
        }
        buf[off] = 0;
        printf("%d: %.*s",e->ident, off, buf);
    }

    if (which & _CC_EVENT_WRITABLE_) {
        printf("%d writeable.", e->ident);
        return false;
    }

    if (which & _CC_EVENT_TIMEOUT_) {
        printf("%d timeout.", e->ident);
        return false;
    }
    return true;
}

void test_event_tcp_listen() {
    struct sockaddr_in sa;
    _cc_event_t *event;
    _cc_async_event_t *async = _cc_get_async_event();

    event = _cc_alloc_event(async, _CC_EVENT_ACCEPT_);
    assert(event != NULL);
    if (event == NULL) {
        return;
    }

    event->timeout = 60000;
    event->callback = _do_event_handler;

    _cc_inet_ipv4_addr(&sa, NULL, port);
    if (!_cc_tcp_listen(async, event, (_cc_sockaddr_t *)&sa, sizeof(struct sockaddr_in))) {
        _cc_free_event(async, event);    
        assert(false);
        return ;
    }
}

int main() {
    int i;
    _cc_alloc_async_event(0, NULL);

    test_event_tcp_listen();

    while((c = getchar()) != 'q') {
        _cc_sleep(100);
    }

    _cc_free_async_event();
    return 0;
}