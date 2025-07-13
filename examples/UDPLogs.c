#include <stdio.h>
#include <libcc/rand.h>
#include <libcc/widgets/event/event.h>
#include <libcc/thread.h>
#include <libcc/logger.h>
#include <libcc/string.h>
#include <locale.h>

static byte_t c = 0;
static _cc_socket_t io_fd = _CC_INVALID_SOCKET_;

int32_t fn_thread(_cc_thread_t *thrd, void* param) {
    struct timeval  tv;
    byte_t sz[_CC_IO_BUFFER_SIZE_];
    _cc_log_info_t *udp_info = (_cc_log_info_t*)&sz;
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
                struct tm* t;

                if (sizeof(_cc_log_info_t) >= n) {
                    continue;
                }

                if (udp_info->version != 1 && udp_info->size < n) {
                    continue;
                }
                t = localtime(&udp_info->timestamp);
                //绿色字
                //SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                //白色字
                //SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                //粉色字
                //SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_INTENSITY);
                
                /*
                "\033[30m 黑色字 \033[0m"
                "\033[31m 红色字 \033[0m"
                "\033[32m 绿色字 \033[0m"
                "\033[33m 黄色字 \033[0m"
                "\033[34m 蓝色字 \033[0m"
                "\033[35m 粉色字 \033[0m"
                "\033[36m 天蓝字 \033[0m"
                "\033[37m 白色字 \033[0m"
                */
                #ifdef __CC_WINDOWS__
                {

                HANDLE ouputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
                COORD xy = {200,200};
                SetConsoleScreenBufferSize(ouputHandle,xy);
                if (udp_info->flags & _CC_LOGGER_FLAGS_DEBUG_) {
                    //天蓝字
                    SetConsoleTextAttribute(ouputHandle, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                } else if (udp_info->flags & _CC_LOGGER_FLAGS_WARNING_) {
                    //黄色字
                    SetConsoleTextAttribute(ouputHandle, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
                } else if (udp_info->flags & _CC_LOGGER_FLAGS_ERROR_) {
                    //红色字
                    SetConsoleTextAttribute(ouputHandle, FOREGROUND_RED | FOREGROUND_INTENSITY);
                } else if (udp_info->flags & _CC_LOGGER_FLAGS_INFO_) {
                    //蓝色字
                    SetConsoleTextAttribute(ouputHandle, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                } else if (udp_info->flags & _CC_LOGGER_FLAGS_NORMAL_) {
                    //白色字
                    SetConsoleTextAttribute(ouputHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                }

                }
                #else
                if (udp_info->flags & _CC_LOGGER_FLAGS_DEBUG_) {
                    puts("\033[36m");
                } else if (udp_info->flags & _CC_LOGGER_FLAGS_WARNING_) {
                    puts("\033[33m");
                } else if (udp_info->flags & _CC_LOGGER_FLAGS_ERROR_) {
                    puts("\033[31m");
                } else if (udp_info->flags & _CC_LOGGER_FLAGS_INFO_) {
                    puts("\033[34m");
                }
                #endif

                
                if (udp_info->flags & _CC_LOGGER_FLAGS_UTF8_) {
#if defined(_CC_UNICODE_) || defined(__CC_WINDOWS__)
                    wchar_t utf16[10240] = {0};
#endif
                    char* s = (char*)(udp_info + 1);
                    *(s + udp_info->size - sizeof(_cc_log_info_t)) = 0;
#if defined(_CC_UNICODE_) || defined(__CC_WINDOWS__)
                    _cc_utf8_to_utf16((const uint8_t*)s, (const uint8_t*)(s + udp_info->size - sizeof(_cc_log_info_t)), (uint16_t *)utf16, (uint16_t *)(utf16 + 10240), 0);
                    wprintf(L"[%4d-%02d-%02d-%02d:%02d:%02d]%s",t->tm_year+1900,t->tm_mon+1,t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec,
                        utf16);
#else
                    printf("[%4d-%02d-%02d-%02d:%02d:%02d]%s", t->tm_year+1900,t->tm_mon+1,t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec,
                        s);
#endif
                } else if (udp_info->flags & _CC_LOGGER_FLAGS_ASIC_) {
                    char* s = (char*)(udp_info + 1);
                    *(s + udp_info->size - sizeof(_cc_log_info_t)) = 0;
                    printf("[%4d-%02d-%02d-%02d:%02d:%02d]%s", t->tm_year+1900,t->tm_mon+1,t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec,
                        s);
                } else if(udp_info->flags & _CC_LOGGER_FLAGS_UTF16_) {
                    wchar_t* s = (wchar_t*)(udp_info + 1);
                    *(s + udp_info->size - sizeof(_cc_log_info_t)) = 0;
                    wprintf(L"[%4d-%02d-%02d-%02d:%02d:%02d]%s", t->tm_year+1900,t->tm_mon+1,t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec,
                        s);
                }

                #ifndef __CC_WINDOWS__
                puts("\033[0m");
                #else
                putchar('\n');
                #endif
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

