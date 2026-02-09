#include <stdarg.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <libcc/buf.h>

void test_alloc_and_free_buf() {
    _cc_buf_t buf;
    assert(_cc_alloc_buf(&buf, 100) == true);
    assert(buf.limit == 100);
    assert(buf.length == 0);
    assert(buf.bytes != NULL);
    assert(_cc_free_buf(&buf) == true);
    printf("alloc and free buf test passed!\n");
}

void test_buf_stringify() {
    _cc_buf_t buf;
    assert(_cc_alloc_buf(&buf, 18) == true);
    const char *str = "test string";
    assert(_cc_buf_appendf(&buf, "%s-test-%d", str,strlen(str)) == true);
    assert(_cc_buf_append(&buf, str, strlen(str)) == true);
    size_t length;
    const tchar_t *result = _cc_buf_stringify(&buf, &length);
    assert(result != NULL);
    assert(length == strlen(str));
    assert(strcmp((const char *)result, str) == 0);
    assert(_cc_free_buf(&buf) == true);
    printf("buf stringify test passed!\n");
}

void test_buf_expand() {
    _cc_buf_t buf;
    assert(_cc_alloc_buf(&buf, 10) == true);
    assert(_cc_buf_expand(&buf, 20) == true);
    assert(buf.limit == 20);
    assert(_cc_free_buf(&buf) == true);
    printf("buf expand test passed!\n");
}

void test_buf_append() {
    _cc_buf_t buf;
    assert(_cc_alloc_buf(&buf, 10) == true);
    const char *data = "test data";
    assert(_cc_buf_append(&buf, data, strlen(data)) == true);
    assert(buf.length == strlen(data));
    assert(memcmp(buf.bytes, data, strlen(data)) == 0);
    assert(_cc_free_buf(&buf) == true);
    printf("buf append test passed!\n");
}

void test_bufA_puts() {
    _cc_buf_t buf;
    assert(_cc_alloc_buf(&buf, 100) == true);
    const char *str = "test string";
    assert(_cc_bufA_puts(&buf, str) == true);
    assert(buf.length == strlen(str));
    assert(memcmp(buf.bytes, str, strlen(str)) == 0);
    assert(_cc_free_buf(&buf) == true);
    printf("bufA puts test passed!\n");
}

void test_bufW_puts() {
    _cc_buf_t buf;
    assert(_cc_alloc_buf(&buf, 100) == true);
    const wchar_t *str = L"test string";
    assert(_cc_bufW_puts(&buf, str) == true);
    assert(buf.length == wcslen(str) * sizeof(wchar_t));
    assert(memcmp(buf.bytes, str, wcslen(str) * sizeof(wchar_t)) == 0);
    assert(_cc_free_buf(&buf) == true);
    printf("bufW puts test passed!\n");
}

void test_buf_from_file() {
    _cc_buf_t buf;
    const char *filename = "test_file.txt";
    FILE *f = fopen(filename, "wb");
    assert(f != NULL);
    const char *content = "test content";
    fwrite(content, 1, strlen(content), f);
    fclose(f);
    assert(_cc_buf_from_file(&buf, filename) == true);
    assert(buf.length == strlen(content));
    assert(memcmp(buf.bytes, content, strlen(content)) == 0);
    assert(_cc_free_buf(&buf) == true);
    remove(filename);
    printf("buf from file test passed!\n");
}

void test_buf_utf_conversion() {
    _cc_buf_t buf;
    assert(_cc_alloc_buf(&buf, 100) == true);
    const char *utf8_str = "test string";
    assert(_cc_buf_append(&buf, utf8_str, strlen(utf8_str)) == true);
    assert(_cc_buf_utf8_to_utf16(&buf, 0) == true);
    assert(_cc_buf_utf16_to_utf8(&buf, 0) == true);
    assert(memcmp(buf.bytes, utf8_str, strlen(utf8_str)) == 0);
    assert(_cc_free_buf(&buf) == true);
    printf("buf utf conversion test passed!\n");
}

int main() {
    test_alloc_and_free_buf();
    test_buf_stringify();
    test_buf_expand();
    test_buf_append();
    test_bufA_puts();
    test_bufW_puts();
    test_buf_from_file();
    test_buf_utf_conversion();
    printf("All buf tests passed!\n");
    return 0;
}