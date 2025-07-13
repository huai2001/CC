#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libcc/socket/socket.h>

#ifndef MSG_WAITALL
#define MSG_WAITALL 0
#endif

int main(int argc, char *const arvg[]) {
    
    struct sockaddr_in srvAddr;
    int i;
    char buff[1024];
    int port = 8088;
    int helloLen = 1024;
    _cc_socket_t sock = _CC_INVALID_SOCKET_;

    if (argc == 2)
        port = atoi(arvg[1]);

    srvAddr.sin_family = AF_INET;
    srvAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    srvAddr.sin_port = htons(port);

    _cc_install_socket();
    sock = socket(AF_INET, SOCK_STREAM, 0);
    connect(sock, (struct sockaddr *)&srvAddr, sizeof(srvAddr));
    strcpy(buff,"test data...");
    helloLen = strlen(buff);
    for (i = 0; i < 100000000; ++i) {
        int size = 0;
        while (size < helloLen) {
            int res = send(sock, buff + size, helloLen - size, 0);
            if (res > 0) size += res;
            else if (res < 0) {
                int e =  _cc_last_errno();
                printf("send error: %d %s\n",e, _cc_last_error(e));
                break;
            }
        }
        recv(sock, buff, helloLen, MSG_WAITALL);
        printf("\r %s %d\n", buff, i);
    }
    while(getchar()!='q'){
    }

    _cc_close_socket(sock);
    _cc_uninstall_socket();
    return 0;
}
