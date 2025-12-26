#include <libcc/sds.h>
#include <libcc/alloc.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

void test_sds_empty_alloc() {
    _cc_sds_t s = _cc_sds_empty_alloc(10);
    assert(s != NULL);
    assert(_cc_sds_length(s) == 10);
    assert(_cc_sds_available(s) == 10);
    _cc_sds_free(s);
    printf("_cc_sds_empty_alloc test passed!\n");
}

void test_sds_alloc() {
    const tchar_t *str = "test string";
    _cc_sds_t s = _cc_sds_alloc(str, strlen(str));
    assert(s != NULL);
    assert(_cc_sds_length(s) == strlen(str));
    assert(strcmp(s, str) == 0);
    _cc_sds_free(s);
    printf("_cc_sds_alloc test passed!\n");
}

void test_sds_format() {
    _cc_sds_t s = _cc_sds_format("%d %s", 123, "test");
    assert(s != NULL);
    assert(strcmp(s, "123 test") == 0);
    _cc_sds_free(s);
    printf("_cc_sds_format test passed!\n");
}

void test_sds_cat() {
    _cc_sds_t s = _cc_sds_alloc("hello ", 6);
    s = _cc_sds_cat(s, "world", 5);
    assert(s != NULL);
    assert(strcmp(s, "hello world") == 0);
    _cc_sds_free(s);

    s = _cc_sds_alloc(NULL, 255);
    s = _cc_sds_cat(s, "hello world", sizeof("hello world") - 1);
    assert(s != NULL);
    assert(strcmp(s, "hello world") == 0);
    _cc_sds_free(s);


    s = _cc_sds_alloc("hello world", 0);
    s = _cc_sds_cat(s, "hello world", sizeof("hello world") - 1);
    assert(s != NULL);
    assert(strcmp(s, "hello worldhello world") == 0);
    _cc_sds_free(s);

    printf("_cc_sds_cat test passed!\n");
}

int main() {
    test_sds_empty_alloc();
    test_sds_alloc();
    test_sds_format();
    test_sds_cat();
    return 0;
}