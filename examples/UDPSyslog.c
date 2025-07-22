#include <stdio.h>
#include <libcc/widgets/event/event.h>
#include <libcc.h>
#include <locale.h>

static byte_t c = 0;

_CC_API_PRIVATE(bool_t) udp(_cc_event_cycle_t *cycle, _cc_event_t *e, uint16_t which) {
    if (which & _CC_EVENT_READABLE_) {
        struct sockaddr_in remote_addr;
        socklen_t addr_len = sizeof(remote_addr);
        char buffer[_CC_32K_BUFFER_SIZE_];
        ssize_t length = recvfrom(e->fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&remote_addr, &addr_len);

        if (length < 0) {
            _cc_logger_error(_T("recvfrom error: %d"), errno);
            return true;
        }

        buffer[length] = '\0';
        _tprintf(_T("Received Syslog: %s\n"), buffer);
    }
    return true;
}

/**/
_CC_API_PRIVATE(int32_t) running(_cc_thread_t *thread, void *args) {
    _cc_event_cycle_t *cycle = (_cc_event_cycle_t *)args;
    while (c != 'q') {
        cycle->wait(cycle, 10);
    }
    cycle->quit(cycle);
    return 1;
}

int main (int argc, char * const argv[]) {
    struct sockaddr_in sin;
    _cc_event_cycle_t cycle;
    _cc_event_t *event;
    _cc_socket_t io_fd = _CC_INVALID_SOCKET_;
    int16_t port = _CC_SYSLOG_PORT_;

    setlocale( LC_CTYPE, "chs" );
    SetConsoleOutputCP(65001);

    _cc_install_socket();

    if (argc == 2) {
        port = atoi(argv[1]);
    }

    io_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(io_fd == -1) {
        printf("socket err\n");
        return -1;
    }

    _cc_inet_ipv4_addr(&sin, nullptr, port);

    if(bind(io_fd, (struct sockaddr *)&sin, sizeof(sin)) == -1){
        fprintf(stderr, "%d bing port error\n", port);
        return -1;
    }

    _tprintf(_T("UDSysPLog Listen Port: %d\n"), port);

    _cc_init_event_select(&cycle);

    event = _cc_event_alloc(&cycle,  _CC_EVENT_READABLE_);
    if (event == nullptr) {
        _cc_close_socket(io_fd);
        return -1;
    }
    event->callback = udp;
    event->fd = io_fd;

    _cc_set_socket_nonblock(io_fd, 1);

    if (!cycle.attach(&cycle, event)) {
        _cc_logger_debug(_T("thread %d add socket (%d) event fial."), _cc_get_thread_id(nullptr), io_fd);
        _cc_free_event(&cycle, event);
        return -1;
    }
    _cc_thread_start(running, "UDPSyslog", &cycle);

    while((c = getchar()) != 'q') {
        if (c == 'c') {
            system("cls");
        }
        _cc_sleep(1000);
    }
    
    _cc_close_socket(io_fd);
    _cc_uninstall_socket();
    return 0;
}

