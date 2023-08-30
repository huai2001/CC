//
//  main.m
//  hook
//
//  Created by CC on 2023/2/23.
//

#import <Foundation/Foundation.h>
#include <libcc.h>
#include "fishhook.h"


void *(*oldConnect)(int, const struct sockaddr *, socklen_t);

void *hookConnect(int socket, const struct sockaddr *address, socklen_t length) {
    tchar_t buf[256];
    int port = 0;
    switch (address->sa_family) {
        case AF_INET: {
            struct sockaddr_in *addr = (struct sockaddr_in*)address;
            port = htons(addr->sin_port);
            _cc_inet_ntop4((byte_t*)&addr->sin_addr, buf, _cc_countof(buf));
        }
            break;
        case AF_INET6: {
            struct sockaddr_in6 *addr = (struct sockaddr_in6*)address;
            port = htons(addr->sin6_port);
            _cc_inet_ntop6((byte_t*)&addr->sin6_addr, buf, _cc_countof(buf));
        }
            break;
    }
    
    NSLog(@"connect %s:%d", buf, port);
    return oldConnect(socket, address, length);
}

void test_connect() {
    _cc_socket_t sock;
    struct sockaddr_in sa;
    _cc_inet_ipv4_addr(&sa, _T("127.0.0.1"), 80);
    /*Open then socket*/
    sock = _cc_socket(AF_INET, _CC_SOCK_NONBLOCK_ | _CC_SOCK_CLOEXEC_ | SOCK_STREAM, 0);
    if (sock == -1) {
        _cc_logger_error(_T("socket fail:%s."), _cc_last_error(_cc_last_errno()));
        return;
    }
    if (connect(sock, (struct sockaddr *)&sa, sizeof(struct sockaddr_in)) == -1) {
        int err = _cc_last_errno();
        if (err != _CC_EINPROGRESS_) {
            _cc_logger_error(_T("connect fail:%s."), _cc_last_error(err));
            return ;
        }
    }
    NSLog(@"connect successful");
    _cc_close_socket(sock);
}

__attribute__((constructor))
static void customHook(int argc, const char **argv) {
    @autoreleasepool {
        NSLog(@"hook successful\n");
        // insert code here...
        //定义数组
        struct rebinding registers[1];
        //定义rebinding结构体
        struct rebinding connect_binding;
        //函数的名称
        connect_binding.name = "connect";
        //新的函数地址
        connect_binding.replacement = hookConnect;
        //保存原始函数地址变量的指针
        connect_binding.replaced = (void *)&oldConnect;
        
        registers[0] = connect_binding;
        rebind_symbols(registers, 1);
    }
}
