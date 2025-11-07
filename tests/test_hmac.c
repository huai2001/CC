#include <libcc/crypto/hmac.h>
#include <libcc/string.h>
#include <libcc/alloc.h>
#include <assert.h>

int main() {
    tchar_t output[256];
    _cc_string_t key = _cc_string("key");
    _cc_string_t data = _cc_string("The quick brown fox jumps over the lazy dog");
    _cc_hmac(_CC_SHA1_, (byte_t*)data.data, data.length, (byte_t*)key.data, key.length, output);
    printf("HMAC-SHA1 = %s\n", output);
    _cc_hmac(_CC_SHA224_, (byte_t*)data.data, data.length, (byte_t*)key.data, key.length, output);
    printf("HMAC-SHA224 = %s\n", output);
    _cc_hmac(_CC_SHA256_, (byte_t*)data.data, data.length, (byte_t*)key.data, key.length, output);
    printf("HMAC-SHA256 = %s\n", output);
    _cc_hmac(_CC_SHA384_, (byte_t*)data.data, data.length, (byte_t*)key.data, key.length, output);
    printf("HMAC-SHA384 = %s\n", output);
    _cc_hmac(_CC_SHA512_, (byte_t*)data.data, data.length, (byte_t*)key.data, key.length, output);
    printf("HMAC-SHA512 = %s\n", output);
    _cc_hmac(_CC_MD5_, (byte_t*)data.data, data.length, (byte_t*)key.data, key.length, output);
    printf("HMAC-MD5 = %s\n", output);
    
    return 0;
}