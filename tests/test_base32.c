#include <libcc/crypto/base32.h>
#include <libcc/alloc.h>
#include <assert.h>

void test_base32_encode() {
    tchar_t output[100] = {0};
    size_t result = _cc_base32_encode((byte_t*)"dsafdsaf", sizeof("dsafdsaf") - 1, output, sizeof(output));
    assert(result > 0);
    assert(output[0] != 0);
    printf("output: %s\n", output);
}

void test_base32_decode() {
    byte_t output[100] = {0};
    size_t result = _cc_base32_decode("MRZWCZTEONQWM===", sizeof("MRZWCZTEONQWM===") - 1, output, sizeof(output));
    assert(result > 0);
    assert(output[0] != 0);
    printf("output: %s\n", output);
}

void test_base32_encode_empty_input() {
    tchar_t output[100] = {0};
    size_t result = _cc_base32_encode(NULL, 0, output, sizeof(output));
    assert(result == 0);
}

void test_base32_decode_invalid_input() {
    const tchar_t input[] = _T("!@#$%^&*()");
    byte_t output[100] = {0};
    size_t result = _cc_base32_decode(input, sizeof(input) - 1, output, sizeof(output));
    assert(result == 0);
}

int main() {
    test_base32_encode();
    test_base32_decode();
    test_base32_encode_empty_input();
    test_base32_decode_invalid_input();
    return 0;
}