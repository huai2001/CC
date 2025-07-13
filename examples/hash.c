#include <stdio.h>
#include <libcc/socket/socket.h>
#include <libcc/alloc.h>
#include <libcc/crc.h>
#include <libcc/time.h>
#include <libcc/md2.h>
#include <libcc/md4.h>
#include <libcc/md5.h>
#include <libcc/hmac.h>

uint8_t* _cc_build_crc8_msb_table(uint8_t polynomial);
uint8_t* _cc_build_crc8_lsb_table(uint8_t polynomial);
#define POLYNOMIAL 0xAB

int main (int argc, tchar_t * const argv[]) {
    char_t *key = (char_t*)"12345678012345678901234567890";
    uint8_t arr[8] = {'1', '2', '3', '4', '5', '6', '7', '8'};
    int32_t i = 0;
    clock_t start, end;
    size_t len,key_length;
    tchar_t *str = _T("abc");
    tchar_t md2[_CC_MD2_DIGEST_LENGTH_ * 2 + 1];
    tchar_t md4[_CC_MD4_DIGEST_LENGTH_ * 2 + 1];
    tchar_t md5[_CC_MD5_DIGEST_LENGTH_ * 2 + 1];
    tchar_t sha1[_CC_SHA1_DIGEST_LENGTH_ * 2 + 1];
    tchar_t sha224[_CC_SHA224_DIGEST_LENGTH_ * 2 + 1];
    tchar_t sha256[_CC_SHA256_DIGEST_LENGTH_ * 2 + 1];
    tchar_t sha384[_CC_SHA384_DIGEST_LENGTH_ * 2 + 1];
    tchar_t sha512[_CC_SHA512_DIGEST_LENGTH_ * 2 + 1];
    /*
    uint8_t *crc8_lsb_table;
    uint8_t *crc8_msb_table;
    crc8_lsb_table = _cc_build_crc8_lsb_table(POLYNOMIAL);
    crc8_msb_table = _cc_build_crc8_msb_table(POLYNOMIAL);
    */
    
    _tprintf(_T("CRC 8-bit lsb table:%x,%x\n"),_cc_crc8(arr, 5, false),_cc_crc8(arr, 5, true));
    
    if (argc > 1 && argv[1][0] != 0) {
        str = (tchar_t*)argv[1];
    }
    
    len = (int32_t)_tcslen(str)*sizeof(tchar_t);
    key_length = (int32_t)_tcslen(key)*sizeof(tchar_t);

    _tprintf("Original:%s\n", str);

    _cc_md2((byte_t*)str, len, md2);
    _cc_md4((byte_t*)str, len, md4);
    _cc_md5((byte_t*)str, len, md5);

    _tprintf(_T("MD2:%s\nMD4:%s\nMD5:%s\n"), md2, md4, md5);

    _cc_sha1((byte_t*)str, len, sha1);
    _cc_sha256((byte_t*)str, len, sha224, true);
    _cc_sha256((byte_t*)str, len, sha256, false);
    _cc_sha512((byte_t*)str, len, sha384, true);
    _cc_sha512((byte_t*)str, len, sha512, false);

    _tprintf(_T("SHA-1:%s\nSHA-224:%s\nSHA-256:%s\nSHA-384:%s\nSHA-512:%s\n"), sha1, sha224, sha256, sha384, sha512);
    
    _cc_hmac(_CC_HMAC_MD5_, (byte_t*)str, len, (const byte_t*)key, key_length, md5);
    _cc_hmac(_CC_HMAC_SHA1_, (byte_t*)str, len, (const byte_t*)key, key_length, sha1);
    _cc_hmac(_CC_HMAC_SHA224_, (byte_t*)str, len, (const byte_t*)key, key_length, sha224);
    _cc_hmac(_CC_HMAC_SHA256_, (byte_t*)str, len, (const byte_t*)key, key_length, sha256);
    _cc_hmac(_CC_HMAC_SHA384_, (byte_t*)str, len, (const byte_t*)key, key_length, sha384);
    _cc_hmac(_CC_HMAC_SHA512_, (byte_t*)str, len, (const byte_t*)key, key_length, sha512);

    _tprintf(_T("HMAC-MD5:%s\nHMAC-SHA-1:%s\nHMAC-SHA-224:%s\nHMAC-SHA-256:%s\nHMAC-SHA-384:%s\nHMAC-SHA-512:%s\n"), md5, sha1, sha224, sha256, sha384, sha512);
    start = clock();
    for (i = 0; i < 1000000; i++) {
        _cc_md4((byte_t*)str, len, md4);
    }
    end = clock();
    printf("_cc_md4 : time span: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);

    start = clock();
    for (i = 0; i < 1000000; i++) {
        _cc_md5((byte_t*)str, len, md5);
    }
    end = clock();
    printf("_cc_md5 : time span: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);
    return 0;
}
