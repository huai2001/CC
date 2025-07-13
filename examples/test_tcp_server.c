#include <libcc.h>
#include <stdio.h>

static bool_t network_event_callback(_cc_event_cycle_t *cycle, _cc_event_t *e, const uint16_t which) {
    if (which & _CC_EVENT_ACCEPT_) {
        _cc_event_t *event;
        _cc_event_cycle_t *cycle_new;
        _cc_socket_t fd;
        _cc_sockaddr_t remote_addr = {0};
        _cc_socklen_t remote_addr_len = sizeof(_cc_sockaddr_t);

        fd = _cc_event_accept(cycle, e, &remote_addr, &remote_addr_len);
        if (fd == _CC_INVALID_SOCKET_) {
            _cc_logger_error(_T("accept fail %s.\n"), _cc_last_error(_cc_last_errno()));
            return true;
        }

        cycle_new = _cc_get_event_cycle();
        event = _cc_event_alloc(cycle_new, _CC_EVENT_TIMEOUT_ | _CC_EVENT_READABLE_ | _CC_EVENT_BUFFER_);
        if (event == nullptr) {
            _cc_close_socket(fd);
            return true;
        }
        _cc_set_socket_nonblock(fd, 1);

        event->fd = fd;
        event->callback = e->callback;
        event->timeout = e->timeout;

        if (cycle_new->attach(cycle_new, event) == false) {
            _cc_logger_debug(_T("thread %d add socket (%d) event fial."), _cc_get_thread_id(nullptr), fd);
            _cc_free_event(event);
            return true;
        }

        {
            struct sockaddr_in *remote_ip = (struct sockaddr_in *)&remote_addr;
            byte_t *ip_addr = (byte_t *)&remote_ip->sin_addr.s_addr;
            _cc_logger_debug(_T("TCP accept [%d,%d,%d,%d] fd:%d"), ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3], fd);
        }

        return true;
    }

    if (which & _CC_EVENT_DISCONNECT_) {
        _cc_logger_debug(_T("%d disconnect to client."), e->fd);
        return false;
    }

    if (which & _CC_EVENT_READABLE_) {

        return true;
    }

    if (which & _CC_EVENT_WRITABLE_) {
        return false;
    }

    if (which & _CC_EVENT_TIMEOUT_) {
        _cc_logger_debug(_T("TCP timeout %d\n"), e->fd);
        return false;
    }

    return true;
}
int main(int argc, char *const argv[]) {
    // char c = 0;
    struct sockaddr_in sa;
    _cc_event_cycle_t cycle;
    _cc_event_t *e;

    _cc_install_socket();

    if (_cc_init_event_poller(&cycle) == false) {
        return 1;
    }
    e = _cc_event_alloc(&cycle, _CC_EVENT_ACCEPT_);
    if (e == nullptr) {
        cycle.delegator.quit(&cycle);
        return -1;
    }
    e->callback = network_event_callback;
    e->timeout = 60000;

    _cc_inet_ipv4_addr(&sa, nullptr, 8080);
    _cc_tcp_listen(&cycle, e, (_cc_sockaddr_t *)&sa, sizeof(struct sockaddr_in));

    while (1) {
        // while((c = getchar()) != 'q') {
        _cc_event_wait(&cycle, 100);
    }

    // cycle.delegator.quit(&cycle);
    return 0;
}
