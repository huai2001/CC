#include <libcc/crypto/base64.h>
#include <libcc/crypto/md5.h>
#include <libcc/crypto/sha.h>
#include <libcc/alloc.h>
#include <libcc/buf.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

void test_base64_encode_decode() {
    const char *input = "Hello, World!";
    size_t input_len = strlen(input);
    size_t encoded_len = _CC_BASE64_EN_LEN(input_len);
    tchar_t *encoded = _cc_malloc(encoded_len + 1);
    size_t decoded_len = _CC_BASE64_DE_LEN(encoded_len);
    byte_t *decoded = _cc_malloc(decoded_len + 1);

    size_t actual_encoded_len = _cc_base64_encode((byte_t *)input, input_len, encoded, encoded_len);
    assert(actual_encoded_len > 0);
    printf("Encoded: %s\n", encoded);

    size_t actual_decoded_len = _cc_base64_decode(encoded, actual_encoded_len, decoded, decoded_len);
    assert(actual_decoded_len > 0);
    printf("Decoded: %s\n", decoded);

    assert(strncmp(input, (char *)decoded, input_len) == 0);

    _cc_free(encoded);
    _cc_free(decoded);
}

void test_base64_encode_empty() {
    const char *input = "";
    size_t input_len = 0;
    size_t encoded_len = _CC_BASE64_EN_LEN(input_len);
    tchar_t *encoded = _cc_malloc(encoded_len + 1);

    size_t actual_encoded_len = _cc_base64_encode((byte_t *)input, input_len, encoded, encoded_len);
    assert(actual_encoded_len == 0);

    _cc_free(encoded);
}

void test_base64_decode_empty() {
    const tchar_t *input = "";
    size_t input_len = 0;
    size_t decoded_len = _CC_BASE64_DE_LEN(input_len);
    byte_t *decoded = _cc_malloc(decoded_len + 1);

    size_t actual_decoded_len = _cc_base64_decode(input, input_len, decoded, decoded_len);
    assert(actual_decoded_len == 0);

    _cc_free(decoded);
}

void test_base64_encode_invalid_output() {
    const char *input = "Test";
    size_t input_len = strlen(input);
    tchar_t *output = NULL;

    size_t actual_encoded_len = _cc_base64_encode((byte_t *)input, input_len, output, 0);
    assert(actual_encoded_len == 0);
}

void test_base64_decode_invalid_output() {
    const tchar_t *input = "VGVzdA==";
    size_t input_len = strlen(input);
    byte_t *output = NULL;

    size_t actual_decoded_len = _cc_base64_decode(input, input_len, output, 0);
    assert(actual_decoded_len == 0);
}

void test_base64_decode_invalid_input() {
    const tchar_t *input = "InvalidBase64!";
    size_t input_len = strlen(input);
    size_t decoded_len = _CC_BASE64_DE_LEN(input_len);
    byte_t *decoded = _cc_malloc(decoded_len + 1);

    size_t actual_decoded_len = _cc_base64_decode(input, input_len, decoded, decoded_len);
    assert(actual_decoded_len == 0);

    _cc_free(decoded);
}

int main() {
    test_base64_encode_decode();
    test_base64_encode_empty();
    test_base64_decode_empty();
    test_base64_encode_invalid_output();
    test_base64_decode_invalid_output();
    test_base64_decode_invalid_input();
    printf("All tests passed!\n");
    return 0;
}