#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libcc.h>
#include <time.h>
#include <cc/socket/socket.h>

#ifndef MSG_WAITALL
#define MSG_WAITALL 0
#endif
int connected_count = 0;

void test_connect(struct sockaddr_in *srvAddr, char* buff, int helloLen, int i) {
    clock_t start, end;
    _cc_socket_t sock;
    int helloLen2 = 1024;
    char buff2[1024];
    start = clock();
     
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if ( sock < 0 ) {
        printf("socket fail: %s : %d\r", _cc_last_error(_cc_last_errno()), connected_count);
        return;
    }
    if (connect(sock, (struct sockaddr *)srvAddr, sizeof(struct sockaddr)) == 0) {
        int size = 0;

        printf("connected\n");

        if (send(sock, buff, helloLen, 0) >= helloLen) {
            if(recv(sock, buff2, helloLen, MSG_WAITALL) < 0) {
                printf("disconnect\n");
            }
        }

        //printf("%d\r", ++connected_count);
        end = clock();
        printf("Connection(%d): %f seconds\n", ++connected_count, (double)(end - start) / CLOCKS_PER_SEC);
        
        _cc_close_socket(sock);
    } else {
        printf("socket connect fail:%s :%d\r", _cc_last_error(_cc_last_errno()), connected_count);
    }
}

int port = 7777;
int nServiceStatus = 1;

static int32_t threadSocketRunning(_cc_thread_t *t, pvoid_t args) {
    int i = 0;
    int helloLen = 1024;
    char buff[1024];
    struct sockaddr_in srvAddr;

    strcpy(buff,"test data...\n");
    helloLen = (int32_t)strlen(buff);

    srvAddr.sin_family = AF_INET;
    srvAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    srvAddr.sin_port = htons(port);

    while (nServiceStatus) {
        test_connect(&srvAddr, buff, helloLen, i);
        _cc_sleep(500);
    }

    return 0;
}

int main(int argc, char *const arvg[]) {
    _cc_thread_t *readwriteThread;
    _cc_install_socket();
    

    if (argc == 2) {
        port = atoi(arvg[1]);
    }

    nServiceStatus = 1;
    readwriteThread = _cc_thread(threadSocketRunning, _T("SocketReadWrite"), NULL);
    if (readwriteThread == NULL) {
        _cc_logger_error(_T("_cc_thread(&ReadWriteThread) fail:%s"), _cc_last_error(_cc_last_errno()));
        nServiceStatus = 0;
    }
     
    while(getchar()!='q'){
        _cc_sleep(1000);
    }

    /*等待线程退出*/
    _cc_wait_thread(readwriteThread, NULL);
    nServiceStatus = 0;

    _cc_uninstall_socket();
    return 0;
}
