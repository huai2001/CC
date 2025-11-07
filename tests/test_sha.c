#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <libcc/crypto/sha.h>
#include <openssl/evp.h>
#include <openssl/err.h>

/* Test cases for SHA-256 functionality */

/**
 * Main function to run the tests.
 */
int main() {
    _cc_string_t tests = _cc_string("The quick brown fox jumps over the lazy dog");
    tchar_t output[_CC_SHA512_DIGEST_LENGTH_ * 2 + 1];
    _cc_sha1((byte_t*)tests.data, tests.length, output);
    printf("SHA-1   = %s\n", output);
    _cc_sha256((byte_t*)tests.data, tests.length, output, false);
    printf("SHA-256 = %s\n", output);
    _cc_sha256((byte_t*)tests.data, tests.length, output, true);
    printf("SHA-224 = %s\n", output);
    _cc_sha512((byte_t*)tests.data, tests.length, output, false);
    printf("SHA-512 = %s\n", output);
    _cc_sha512((byte_t*)tests.data, tests.length, output, true);
    printf("SHA-384 = %s\n", output);

    printf("All tests passed!\n");
    return 0;
}