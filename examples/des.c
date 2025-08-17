#include <stdio.h>
#include <libcc/alloc.h>
#include <libcc/des.h>
#include <libcc/base64.h>

int main (int argc, char * const argv[]) {
    int i = 0;
    char_t k[9] = "12345678";
    char_t s[9] = "12345678";
    byte_t o[8] = {0};
    byte_t o2[8] = {0};
    int32_t outlen = 64;
    uchar_t out[14];

    _cc_des_t des_enc;
    _cc_des_t des_dec;

    _cc_des3_t des3_enc;
    _cc_des3_t des3_dec;

    _cc_des_init(&des_enc);
    _cc_des_init(&des_dec);

    _tprintf(_T("src:"), o);

    for (i = 0; i < 8; i++) {
        _tprintf(_T("%02x "), s[i]);
    }
    _puttchar('\n');

    _cc_des_setkey_enc(&des_enc, (byte_t*)k);
    _cc_des_setkey_dec(&des_dec, (byte_t*)k);
    _cc_des_crypt_ecb(&des_enc, (byte_t*)s, o);


    _tprintf(_T("des_enc:"), o);

    for (i = 0; i < 8; i++) {
        _tprintf(_T("%02x "), o[i]);
    }
    _puttchar('\n');
    _cc_base64_encode((byte_t*)o, 8, out, &outlen);

    _cc_des_crypt_ecb(&des_dec, (byte_t*)o, o);
    _tprintf(_T("des_dec:"), o);

    for (i = 0; i < 8; i++) {
        _tprintf(_T("%02x "), o[i]);
    }
    _puttchar('\n');

    _tprintf(_T("des: %s\n"), out);


    _cc_des3_init(&des3_enc);
    _cc_des3_init(&des3_dec);
    _cc_des3_set2key_enc(&des3_enc, (byte_t*)k);
    _cc_des3_set2key_dec(&des3_dec, (byte_t*)k);
    _cc_des3_crypt_ecb(&des3_enc, (byte_t*)s, o);


    _tprintf(_T("des3_enc:"), o);

    for (i = 0; i < 8; i++) {
        _tprintf(_T("%02x "), o[i]);
    }
    _puttchar('\n');
    _cc_base64_encode((byte_t*)o, 8, out, &outlen);

    _cc_des3_crypt_ecb(&des3_dec, (byte_t*)o, o);
    _tprintf(_T("des3_dec:"), o);

    for (i = 0; i < 8; i++) {
        _tprintf(_T("%02x "), o[i]);
    }
    _puttchar('\n');

    _tprintf(_T("des3: %s\n"), out);

    system("pause");
    return 0;
}
