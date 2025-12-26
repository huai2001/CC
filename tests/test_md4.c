#include <libcc/crypto/md4.h>
#include <libcc/string.h>
#include <libcc/alloc.h>
#include <assert.h>

void test_md4_init() {
    _cc_md4_t ctx;
    _cc_md4_init(&ctx);
    assert(ctx.state[0] == 0x67452301);
    assert(ctx.state[1] == 0xEFCDAB89);
    assert(ctx.state[2] == 0x98BADCFE);
    assert(ctx.state[3] == 0x10325476);
    assert(ctx.total[0] == 0);
    assert(ctx.total[1] == 0);
}

void test_md4_update_empty() {
    _cc_md4_t ctx;
    _cc_md4_init(&ctx);
    _cc_md4_update(&ctx, NULL, 0);
    assert(ctx.total[0] == 0);
    assert(ctx.total[1] == 0);
}

void test_md4_update_small_input() {
    _cc_md4_t ctx;
    _cc_md4_init(&ctx);
    const byte_t input[] = "test";
    _cc_md4_update(&ctx, input, sizeof(input) - 1);
    assert(ctx.total[0] == sizeof(input) - 1);
    assert(ctx.total[1] == 0);
}

void test_md4_update_large_input() {
    _cc_md4_t ctx;
    _cc_md4_init(&ctx);
    byte_t input[1024];
    memset(input, 'A', sizeof(input));
    _cc_md4_update(&ctx, input, sizeof(input));
    assert(ctx.total[0] == sizeof(input));
    assert(ctx.total[1] == 0);
}

void test_md4_final() {
    _cc_md4_t ctx;
    _cc_md4_init(&ctx);
    const byte_t input[] = "test";
    _cc_md4_update(&ctx, input, sizeof(input) - 1);
    byte_t output[_CC_MD4_DIGEST_LENGTH_];
    _cc_md4_final(&ctx, output);
    // Verify the output is not zeroed
    for (size_t i = 0; i < _CC_MD4_DIGEST_LENGTH_; i++) {
        assert(output[i] != 0);
    }
}

void test_md4_file() {
    tchar_t output[_CC_MD4_DIGEST_LENGTH_ * 2 + 1];
    FILE *fp = fopen("test_file.txt", "wb");
    assert(fp != NULL);
    const byte_t content[] = "test content";
    fwrite(content, sizeof(byte_t), sizeof(content) - 1, fp);
    fclose(fp);

    assert(_cc_md4file(_T("test_file.txt"), output));
    remove("test_file.txt");
}

void test_md4_invalid_file() {
    tchar_t output[_CC_MD4_DIGEST_LENGTH_ * 2 + 1];
    assert(!_cc_md4file(_T("nonexistent_file.txt"), output));
}

int main() {
    test_md4_init();
    test_md4_update_empty();
    test_md4_update_small_input();
    test_md4_update_large_input();
    test_md4_final();
    test_md4_file();
    test_md4_invalid_file();
    return 0;
}