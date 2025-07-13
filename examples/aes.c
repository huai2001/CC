#include <stdio.h>
#include <libcc/alloc.h>
#include <libcc/aes.h>
#include <libcc/base64.h>

int main (int argc, char * const argv[]) {
    int i = 0;
    char_t k[17] = "1234567812345678";
    char_t s[17] = "1234567812345678";
    byte_t o[16] = {0};
    byte_t o2[16] = {0};
    int32_t outlen = 64;
    uchar_t out[14];

    _cc_aes_t aes_enc;
    _cc_aes_t aes_dec;


    _cc_aes_init(&aes_enc);
    _cc_aes_init(&aes_dec);

    _tprintf(_T("aes_src:"), o);

    for (i = 0; i < 16; i++) {
        _tprintf(_T("%02x "), s[i]);
    }
    _puttchar('\n');

    _cc_aes_setkey_enc(&aes_enc, (byte_t*)k,256);
    _cc_aes_setkey_dec(&aes_dec, (byte_t*)k,256);
    _cc_aes_crypt_ecb(&aes_enc, _CC_AES_ENCRYPT_, (byte_t*)s, o);


    _tprintf(_T("aes_enc:"), o);

    for (i = 0; i < 8; i++) {
        _tprintf(_T("%02x "), o[i]);
    }
    _puttchar('\n');
    _cc_base64_encode((byte_t*)o, 8, out, outlen);

    _cc_aes_crypt_ecb(&aes_dec, _CC_AES_DECRYPT_, (byte_t*)o, o);
    _tprintf(_T("aes_dec:"), o);

    for (i = 0; i < 16; i++) {
        _tprintf(_T("%02x "), o[i]);
    }
    _puttchar('\n');

    _tprintf(_T("des: %s\n"), out);

    system("pause");
    return 0;
}
