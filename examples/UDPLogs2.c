#include <stdio.h>
#include <libcc/rand.h>
#include <libcc/widgets/event/event.h>
#include <libcc/thread.h>
#include <libcc/logger.h>
#include <libcc/string.h>
#include <libcc/base64.h>
#include <locale.h>
#include <zlib/zlib.h>

static byte_t c = 0;
static _cc_socket_t io_fd = _CC_INVALID_SOCKET_;

int32_t fn_thread(_cc_thread_t *thrd, void* param) {
    struct timeval  tv;
    byte_t sz[_CC_IO_BUFFER_SIZE_];
    int32_t n = 0, res = 0;
    fd_set  read_set;
    //fd_set  write_set;
    //fd_set  err_set;

    while(c != 'q') {
        FD_ZERO(&read_set);
        //FD_ZERO(&write_set);
        //FD_ZERO(&err_set);
        FD_SET(io_fd, &read_set);
        //FD_SET(io_fd, &write_set);
        //FD_SET(io_fd, &err_set);

        tv.tv_sec = 0;
        tv.tv_usec = 100000;
#ifndef __CC_WINDOWS__
        res = select((int)(io_fd+1), &read_set, nullptr, nullptr, &tv);
#else
        res = select(0, &read_set, nullptr, nullptr, &tv);
#endif
        if (res == 0) {
            continue;
        }
        if (res < 0) {
            int32_t lerrno = _cc_last_errno();
            if (lerrno != _CC_EINTR_) {
                _tprintf(_T("error:%d, %s"), lerrno, _cc_last_error(lerrno));
                c = 'q';
                return 1;
            }
            continue;
        }

        if (res > 0 && FD_ISSET(io_fd, &read_set)) {
            struct sockaddr_in c_sin;
            socklen_t c_addr_len = (socklen_t)sizeof(c_sin);
            n = (int32_t)recvfrom(io_fd, (char*)&sz, _CC_IO_BUFFER_SIZE_, 0, (struct sockaddr *)&c_sin, &c_addr_len);
            if (n > 0) {
                byte_t decmp[_CC_IO_BUFFER_SIZE_];

                sz[n] = 0;
                uLong clen = _CC_IO_BUFFER_SIZE_;

                if (n > 0 && uncompress(decmp, &clen, sz, n) == Z_OK) {
                    decmp[clen] = 0;
                    _tprintf(_T("dec:%s\n"), decmp);
                } else {
                    _tprintf(_T("log:%s\n"), sz);
                }
                //_tprintf(_T("log:%s\n"), sz);
            }
        }
    }
    return 0;
}


int main (int argc, char * const argv[]) {
    struct sockaddr_in sin;
    int16_t port = 8600;

    setlocale( LC_CTYPE, "chs" );

    _cc_install_socket();

    if (argc == 2) {
        port = atoi(argv[1]);
    }

    io_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(io_fd == -1){
        printf("socket err\n");
        return -1;
    }

    _cc_inet_ipv4_addr(&sin, nullptr, port);
    if(bind(io_fd, (struct sockaddr *)&sin, sizeof(sin)) == -1){
        fprintf(stderr, "%s\n", "bing port error\n");
        return -1;
    }

    _tprintf(_T("UDPLogs Listen Port: %d\n"), port);
    
    _cc_thread_start(fn_thread, "UDPLogs", nullptr);

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

