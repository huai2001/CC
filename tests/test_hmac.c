#include <libcc/crypto/hmac.h>
#include <libcc/string.h>
#include <libcc/alloc.h>
#include <assert.h>

int main() {
    tchar_t output[256];
    const tchar_t *key = "key";
    size_t key_length = 3;
    const tchar_t *data = "The quick brown fox jumps over the lazy dog";
    const size_t length = sizeof("The quick brown fox jumps over the lazy dog") - 1;
    _cc_hmac(_CC_SHA1_, (byte_t*)data, length, (byte_t*)key, key_length, output);
    printf("HMAC-SHA1 = %s\n", output);
    _cc_hmac(_CC_SHA224_, (byte_t*)data, length, (byte_t*)key, key_length, output);
    printf("HMAC-SHA224 = %s\n", output);
    _cc_hmac(_CC_SHA256_, (byte_t*)data, length, (byte_t*)key, key_length, output);
    printf("HMAC-SHA256 = %s\n", output);
    _cc_hmac(_CC_SHA384_, (byte_t*)data, length, (byte_t*)key, key_length, output);
    printf("HMAC-SHA384 = %s\n", output);
    _cc_hmac(_CC_SHA512_, (byte_t*)data, length, (byte_t*)key, key_length, output);
    printf("HMAC-SHA512 = %s\n", output);
    _cc_hmac(_CC_MD5_, (byte_t*)data, length, (byte_t*)key, key_length, output);
    printf("HMAC-MD5 = %s\n", output);
    
    return 0;
}