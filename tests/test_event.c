#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <libcc/thread.h>
#include <libcc/event.h>
#include <libcc/timeout.h>
static int c = 0;
static uint16_t port = 3000;

#define TEST_CASE(name) printf("Running test: %s\n", #name); name();

void test_accept(_cc_async_event_t *async, _cc_event_t *e) {
    _cc_socket_t fd;
    _cc_event_t *event;
    struct sockaddr_in remote_addr = {0};
    _cc_socklen_t remote_addr_len = sizeof(struct sockaddr_in);
    _cc_async_event_t *async2 = _cc_get_async_event();

    fd = async->accept(async, e, (_cc_sockaddr_t *)&remote_addr, &remote_addr_len);
    if (fd == _CC_INVALID_SOCKET_) {
        _tprintf("thread %d accept fail %s.", _cc_get_thread_id(NULL), _cc_last_error(_cc_last_errno()));
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
        _cc_logger_error("thread %d add socket (%d) event fial.", _cc_get_thread_id(NULL), fd);
        _cc_free_event(async2, event);
        return ;
    }
	_cc_logger_debug("%d accept.\n", event->ident & 0xFFFF);
}

static int times = 0;

static bool_t test_event_callback(_cc_async_event_t *async, _cc_event_t *e, const uint32_t which) {
    if (which & _CC_EVENT_ACCEPT_) {
        test_accept(async,e);
        return true;
    } else if (which & _CC_EVENT_CONNECT_) {
        _cc_logger_debug("%d connected.\n", e->ident & 0xFFFF);
        return true;
    }

	if (which & _CC_EVENT_CLOSED_) {
        _cc_logger_debug("%d disconnect.", e->ident & 0xFFFF);
        return false;
    }

    if (which & _CC_EVENT_READABLE_) {
        byte_t buf[_CC_IO_BUFFER_SIZE_];
        int off = _cc_recv(e->fd, buf, _cc_countof(buf));
        if (off < 0) {
            _cc_logger_error("%d recv fail.", e->ident & 0xFFFF);
            return false;
        } else if (off == 0) {
            _cc_logger_error("%d client close.", e->ident & 0xFFFF);
            return false;
        }
        if (_strnicmp((char_t*)buf, "ping", 5) == 0){
            if (_cc_send(e->fd, (byte_t*)"pong", 5) < 0) {
                _cc_logger_error("%d send pong fail.", e->ident & 0xFFFF);
                return false;
            }
        } else if (_strnicmp((char_t*)buf, "close", 5) == 0){
            _cc_logger_info("%d client close.", e->ident & 0xFFFF);
            return false;
        }
        buf[off] = 0;
        //_tprintf("%d: %.*s",e->ident & 0xFFFF, off, buf);
    }

    if (which & _CC_EVENT_WRITABLE_) {
        _cc_logger_error("%d writeable.", e->ident & 0xFFFF);
        return false;
    }

    if (which & _CC_EVENT_TIMEOUT_) {
        //_tprintf("%d timeout.", e->ident & 0xFFFF);
        if (times++ > 10000) {
            if (_cc_send(e->fd, (byte_t*)"close", 5) < 0) {
                _cc_logger_error("%d send close fail.", e->ident & 0xFFFF);
                return false;
            }
        } else if (_cc_send(e->fd, (byte_t*)"ping", 5) < 0) {
            _cc_logger_error("%d send ping fail.", e->ident & 0xFFFF);
            return false;
        }
    }
    return true;
}
static bool_t test_event_timeout_callback(_cc_async_event_t *async, _cc_event_t *e, const uint32_t which) {
    if (which & _CC_EVENT_TIMEOUT_) {
        _cc_logger_error("%d timer timeout. %ld", e->ident & 0xFFFF, e->data);
        return false;
    }

    if (which & _CC_EVENT_CLOSED_) {
        _cc_logger_debug("%d destroy timeout. %ld", e->ident & 0xFFFF, e->data);
    }       
    return false;
}

void test_buffer_allocation() {
    _cc_io_buffer_t *buffer = _cc_alloc_io_buffer(_CC_IO_BUFFER_SIZE_,NULL);
    assert(buffer != NULL);
    _cc_free_io_buffer(buffer);
}

void test_event_timeout() {
    _cc_event_t *event;
    _cc_async_event_t *async = _cc_get_async_event();
    assert(async != NULL);

    event = _cc_add_event_timeout(async, 6000, test_event_timeout_callback, 0);
    assert(event != NULL);
    _CC_UNUSED(event);
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
    event->callback = test_event_callback;

    _cc_inet_ipv4_addr(&sa, NULL, port);
    if (!_cc_tcp_listen(async, event, (_cc_sockaddr_t *)&sa, sizeof(struct sockaddr_in))) {
        _cc_free_event(async, event);    
        assert(false);
        return ;
    }
}

void test_event_tcp_connect() {
    struct sockaddr_in sa;
    _cc_event_t *event;
    _cc_async_event_t *async = _cc_get_async_event();
    assert(async != NULL);

    event = _cc_alloc_event(async, _CC_EVENT_CONNECT_|_CC_EVENT_TIMEOUT_|_CC_EVENT_READABLE_);

    assert(event != NULL);
    if (event == NULL) {
        return;
    }

    event->timeout = 6000;
    event->callback = test_event_callback;

    _cc_inet_ipv4_addr(&sa, "127.0.0.1", port);
    if (!_cc_tcp_connect(async, event, (_cc_sockaddr_t *)&sa, sizeof(struct sockaddr_in))) {
        _cc_free_event(async, event);
        return;
    }
}

int main() {
    int i;
    _cc_alloc_async_event(0, NULL);
    _cc_logger_dump();
	
    printf("sizeof(_cc_event_t) == %ld\nRunning tests...\n", sizeof(_cc_event_t));
    TEST_CASE(test_buffer_allocation);
    TEST_CASE(test_event_tcp_listen);
    TEST_CASE(test_event_tcp_connect);
    TEST_CASE(test_event_timeout);

    for (i = 0; i < 100; i++) {
        test_event_tcp_connect();
    }

    while((c = getchar()) != 'q') {
        _cc_sleep(100);
    }

    printf("All tests passed!\n");
    _cc_free_async_event();
    return 0;
}