#include <libcc/string.h>
#include <libcc/platform.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

void test_cc_hex16() {
    const tchar_t *hex_str = "0123456789ABCDEF";
    uint64_t result = _cc_hex16(hex_str);
    assert(result == 0x0123456789ABCDEF);
    printf("_cc_hex16 test passed!\n");
}

void test_cc_hex8() {
    const tchar_t *hex_str = "01234567";
    uint32_t result = _cc_hex8(hex_str);
    assert(result == 0x01234567);
    printf("_cc_hex8 test passed!\n");
}

void test_cc_hex4() {
    const tchar_t *hex_str = "0123";
    uint16_t result = _cc_hex4(hex_str);
    assert(result == 0x0123);
    printf("_cc_hex4 test passed!\n");
}

void test_cc_hex2() {
    const tchar_t *hex_str = "01";
    uint8_t result = _cc_hex2(hex_str);
    assert(result == 0x01);
    printf("_cc_hex2 test passed!\n");
}

void test_cc_bytes2hex() {
    const byte_t bytes[] = {0x01, 0x23, 0x45, 0x67};
    tchar_t hex_str[9];
    size_t result = _cc_bytes2hex(bytes, sizeof(bytes), hex_str, sizeof(hex_str));
    assert(result == 8);
    assert(strcmp(hex_str, "01234567") == 0);
    printf("_cc_bytes2hex test passed!\n");
}

void test_cc_hex2bytes() {
    const tchar_t *hex_str = "01234567";
    byte_t bytes[4];
    size_t result = _cc_hex2bytes(hex_str, strlen(hex_str), bytes, sizeof(bytes));
    assert(result == 4);
    assert(bytes[0] == 0x01);
    assert(bytes[1] == 0x23);
    assert(bytes[2] == 0x45);
    assert(bytes[3] == 0x67);
    printf("_cc_hex2bytes test passed!\n");
}

void test_cc_trimA_copy() {
    const char_t *src = "  test string  ";
    char_t dst[20];
    size_t result = _cc_trimA_copy(dst, sizeof(dst), src, strlen(src));
    assert(result == 11);
    assert(strcmp(dst, "test string") == 0);
    printf("_cc_trimA_copy test passed!\n");
}

void test_cc_trimW_copy() {
    const wchar_t *src = L"  test string  ";
    wchar_t dst[20];
    size_t result = _cc_trimW_copy(dst, sizeof(dst), src, wcslen(src));
    assert(result == 11);
    assert(wcscmp(dst, L"test string") == 0);
    printf("_cc_trimW_copy test passed!\n");
}

void test_cc_substr() {
    const tchar_t *src = "test string";
    tchar_t dst[20];
    tchar_t *result = _cc_substr(dst, src, 2, 4);
    assert(result != nullptr);
    assert(strcmp(result, "st s") == 0);
    printf("_cc_substr test passed!\n");
}

void test_cc_to_number() {
    const tchar_t *src = "123.45e-2";
    _cc_number_t num;
    const tchar_t *result = _cc_to_number(src, &num);
    assert(result != nullptr);
    assert(num.vt == _CC_NUMBER_FLOAT_);
    assert(num.v.uni_float == 1.2345);
    printf("_cc_to_number test passed!\n");
}

static const tchar_t* fn(const tchar_t *ptr, int32_t *offset) {
    *offset = 2;//sizeof("||") - 1;
    return _tcsstr(ptr,"||");
}

void test_cc_split() {
    const tchar_t *src = "123||456||898||232||";
    _cc_string_t sp[6];
    int32_t i;
    int32_t count = _cc_split(sp,_cc_countof(sp),src, fn);

    assert(count != 4);
    
    for (i = 0; i < count; i++) {
        _cc_string_t *r = &sp[i];
        printf("split:%d, %.*s\n",i,(int)r->length,r->data);
    }
    printf("_cc_cc_split test passed!\n");
}
int main() {
    test_cc_hex16();
    test_cc_hex8();
    test_cc_hex4();
    test_cc_hex2();
    test_cc_bytes2hex();
    test_cc_hex2bytes();
    test_cc_trimA_copy();
    test_cc_trimW_copy();
    test_cc_substr();
    test_cc_to_number();
    test_cc_split();
    return 0;
}