#include <libcc/crypto/base16.h>
#include <libcc/string.h>
#include <assert.h>
#include <stdio.h>

void test_base16_encode() {
    const byte_t input[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    tchar_t output[17];
    size_t result = _cc_base16_encode(input, sizeof(input), output, sizeof(output));
    assert(result == 16);
    assert(_tcscmp(output, _T("0123456789ABCDEF")) == 0);
    printf("test_base16_encode passed\n");
}

void test_base16_encode_null_output() {
    const byte_t input[] = {0x01, 0x23, 0x45, 0x67};
    size_t result = _cc_base16_encode(input, sizeof(input), NULL, 0);
    assert(result == 0);
    printf("test_base16_encode_null_output passed\n");
}

void test_base16_encode_insufficient_output() {
    const byte_t input[] = {0x01, 0x23, 0x45, 0x67};
    tchar_t output[5];
    size_t result = _cc_base16_encode(input, sizeof(input), output, sizeof(output));
    assert(result == (size_t)-1);
    printf("test_base16_encode_insufficient_output passed\n");
}

void test_base16_decode() {
    const tchar_t input[] = _T("0123456789ABCDEF");
    byte_t output[8];
    size_t result = _cc_base16_decode(input, _tcslen(input), output, sizeof(output));
    assert(result == 8);
    assert(memcmp(output, "\x01\x23\x45\x67\x89\xAB\xCD\xEF", 8) == 0);
    printf("test_base16_decode passed\n");
}

void test_base16_decode_null_output() {
    const tchar_t input[] = _T("0123456789ABCDEF");
    size_t result = _cc_base16_decode(input, _tcslen(input), NULL, 0);
    assert(result == 0);
    printf("test_base16_decode_null_output passed\n");
}

void test_base16_decode_insufficient_output() {
    const tchar_t input[] = _T("0123456789ABCDEF");
    byte_t output[4];
    size_t result = _cc_base16_decode(input, _tcslen(input), output, sizeof(output));
    assert(result == (size_t)-1);
    printf("test_base16_decode_insufficient_output passed\n");
}

void test_base16_decode_invalid_input() {
    const tchar_t input[] = _T("GHIJKLMNOPQRSTUV");
    byte_t output[8];
    size_t result = _cc_base16_decode(input, _tcslen(input), output, sizeof(output));
    assert(result == 0);
    printf("test_base16_decode_invalid_input passed\n");
}

int main() {
    test_base16_encode();
    test_base16_encode_null_output();
    test_base16_encode_insufficient_output();
    test_base16_decode();
    test_base16_decode_null_output();
    test_base16_decode_insufficient_output();
    test_base16_decode_invalid_input();
    return 0;
}