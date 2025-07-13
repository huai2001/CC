#include <libcc.h>
#include <libcc/widgets/ip_locator.h>


uint32_t str2ip(const char_t *cs) {
    char_t *result = nullptr;
    uint32_t res = 0;
    char_t *s = strdup(cs);
    result = strtok( s, "." );
    while( result ) {
        res <<= 8;
        res |= (uint32_t)atoi(result);
        result = strtok( nullptr, "." );
    }
    free(s);
    return res;
}

/**
 * Make sure 'buf' is big enough....
 */
char_t* ip2str(uint32_t ip, char_t *buf) {
    sprintf(buf, "%d.%d.%d.%d", ip >> 24, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff );
    return buf;
}

static _cc_ip_locator_t locator;
static byte_t addr[256];
static byte_t addr_gbk[256];

void ListenUDP(uint16_t port) {
    struct sockaddr_in sin;
    int32_t n = 0, res = 0, len = 0;
    struct timeval tv;
    fd_set  read_set;
    byte_t sz[_CC_IO_BUFFER_SIZE_];

    _cc_socket_t io_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (io_fd == -1) {
        printf("socket err\n");
        return ;
    }

    _cc_inet_ipv4_addr(&sin, nullptr, port);
    if(bind(io_fd, (struct sockaddr *)&sin, sizeof(sin)) == -1){
        fprintf(stderr, "%s\n", "bing port error\n");
        return ;
    }

    while(1) {
        FD_ZERO(&read_set);
        FD_SET(io_fd, &read_set);

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
                return ;
            }
            continue;
        }

        if (res > 0 && FD_ISSET(io_fd, &read_set)) {
            struct sockaddr_in c_sin;
            socklen_t c_addr_len = (socklen_t)sizeof(c_sin);
            n = (int32_t)recvfrom(io_fd, (char*)&sz, _CC_IO_BUFFER_SIZE_, 0, (struct sockaddr *)&c_sin, &c_addr_len);
            if (n > 0) {
                len = locator.query(&locator, str2ip((char*)sz), addr_gbk, 256);
                len = _cc_gbk_to_utf8(addr_gbk, addr_gbk + len, addr, &addr[256]);
                addr[len] = 0;
                sendto(io_fd, (char*)&addr, len + sizeof(tchar_t), 0, (struct sockaddr *)&c_sin, c_addr_len);
                printf("%s %d\n", addr, len);
            }
        }
    }

    //_cc_close_socket(io_fd);
}

int main(int argc, const char * argv[]) {
    // insert code here...
    uint16_t port = 8600;
    int len;
    tchar_t path[_CC_MAX_PATH_];// = "/Users/Github/CC/widgets/libIPLocator/qqwry.dat";
    tchar_t qqwry[_CC_MAX_PATH_ * 2];

    _cc_install_socket();

    if (argc == 2) {
        port = atoi(argv[1]);
    }
    _cc_get_base_path(path,_CC_MAX_PATH_);
    
    _sntprintf(,_cc_countof(qqwry),_T("%s/qqwry.dat"),path);
    _cc_init_ip_locator(&locator, qqwry);
    
    len = locator.get_version(&locator, addr_gbk, 256);
    _cc_gbk_to_utf8(addr_gbk, addr_gbk + len, addr, &addr[256]);
    
    printf("%s\n",(char*)addr);

    ListenUDP(port);
    
    locator.quit(&locator);
    _cc_uninstall_socket();
    return 0;
}
