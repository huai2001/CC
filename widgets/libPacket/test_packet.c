
#include <cc/alloc.h>
#include <cc/core.h>
#include <cc/dirent.h>

#include "packet.h"

#define MAX_DATA 1024

typedef struct _test_packet {
    uint8_t a;
    uint16_t b;
    byte_t buf[MAX_DATA];
    uint32_t c;
    uint64_t d;
} _test_packet;

int testes() {
    _test_packet test = {0};
    _test_packet* ptest = NULL;
    _cc_tcp_t encryptPacket;
    _cc_tcp_t decryptPacket;

    int i = 0;

    for (i = 0; i < MAX_DATA; i++) {
        test.buf[i] = rand() % 256;
        // printf("%.2x,",test.buf[i]);
    }
    test.a = rand() % 256;
    test.b = 2;
    test.c = 255;
    test.d = 1000;
    // printf("test:%d,%d,%d,%lld\n", test.a,test.b,test.c, test.d);

    _cc_tcp_encrypt(1, 2, (byte_t*)&test, sizeof(test),
                    _CC_PACKET_KIND_MAPPED_ | _CC_PACKET_KIND_COMPRESS_,
                    &encryptPacket);
    // printf("Size:%d-%ld\n", encryptPacket.header.length, sizeof(test));
    i = _cc_tcp_decrypt((const byte_t*)&encryptPacket, sizeof(_cc_tcp_t),
                        &decryptPacket);
    if (!i) {
        printf("%d\n", i);
        return 1;
    }

    // printf("cmd:%d,sid:%d\n", decryptPacket.head.cmd.mid,
    // decryptPacket.head.cmd.sid);
    ptest = (_test_packet*)decryptPacket.buffer;
    if (memcmp(ptest, &test, sizeof(_test_packet)) != 0) {
        return 2;
    }
    // printf("test:%d,%d,%d,%lld\n", ptest->a,ptest->b,ptest->c, ptest->d);
    // printf("\n");
    //_cc_sleep(1);
    return 0;
}

int main(int argc, char const* argv[]) {
    int i = 0;
    clock_t start, end;
    srand((uint32_t)time(NULL));
    _cc_sleep(100);

    start = clock();
    for (i = 0; i < 100000; i++) {
        switch (testes()) {
            case 1:
                printf("%d decryptTCPPacket fail.\n", i);
                break;

            case 2:
                printf("%d data error.\n", i);
                break;
            default:
                break;
        }
    }
    end = clock();
    printf("%d testes done.\n", i);
    printf("testes TCPPacket: %f seconds\n",
           (double)(end - start) / CLOCKS_PER_SEC);
    return 0;
}
