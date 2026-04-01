#include <stdio.h>
#include "quicklz.h"

struct _testes{
    int a;
    char c[256];
    int b;
};

int testes_compress() {
    size_t d = 0;
    qlz_state_compress state_compress;
    qlz_state_decompress state_decompress;
    char compressed[sizeof(struct _testes) + 400];
    char uncompressed[sizeof(struct _testes) + 400];
    struct _testes testes;
    struct _testes *un_testes;

    testes.a = 100;
    testes.b = 200;
    memset(testes.c, 0, sizeof(testes.c));
    memset(&state_compress, 0, sizeof(qlz_state_compress));
    memset(&state_decompress, 0, sizeof(qlz_state_decompress));
    strcpy(testes.c, "testes");
    
    d = qlz_compress(&testes, (char*)&compressed, sizeof(struct _testes), &state_compress);
    
    printf("qlz compress %ld\n", d);
    
    d = qlz_decompress((char*)&compressed, (char*)&uncompressed, &state_decompress);
    un_testes = (struct _testes*)&uncompressed;
    
    printf("qlz compress %ld, %d - %d\n", d, un_testes->a, un_testes->b);

    return 0;
}
