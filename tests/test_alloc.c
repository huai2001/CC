#include <libcc/alloc.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

void test_malloc_and_free() {
    void *ptr = _cc_malloc(100);
    assert(ptr != NULL);
    _cc_free(ptr);
    printf("malloc and free test passed!\n");
}

void test_calloc_and_free() {
    void *ptr = _cc_calloc(10, 10);
    assert(ptr != NULL);
    _cc_free(ptr);
    printf("calloc and free test passed!\n");
}

void test_realloc_and_free() {
    void *ptr = _cc_malloc(100);
    assert(ptr != NULL);
    ptr = _cc_realloc(ptr, 200);
    assert(ptr != NULL);
    _cc_free(ptr);
    printf("realloc and free test passed!\n");
}

void test_malloc_zero_size() {
    void *ptr = _cc_malloc(0);
    assert(ptr == NULL);
    printf("malloc zero size test passed!\n");
}

void test_realloc_zero_size() {
    void *ptr = _cc_malloc(100);
    assert(ptr != NULL);
    ptr = _cc_realloc(ptr, 0);
    assert(ptr == NULL);
    printf("realloc zero size test passed!\n");
}

void test_free_nullptr() {
    _cc_free(NULL);
    printf("free NULL test passed!\n");
}

void test_strdupA() {
    const char *str = "test string";
    char *dup = _cc_strdupA(str);
    assert(dup != NULL);
    assert(strcmp(str, dup) == 0);
    _cc_free(dup);
    printf("strdupA test passed!\n");
}

void test_strdupW() {
    const wchar_t *str = L"test string";
    wchar_t *dup = _cc_strdupW(str);
    assert(dup != NULL);
    assert(wcscmp(str, dup) == 0);
    _cc_free(dup);
    printf("strdupW test passed!\n");
}

void test_strndupA() {
    const char *str = "test string";
    char *dup = _cc_strndupA(str, 4);
    assert(dup != NULL);
    assert(strncmp(str, dup, 4) == 0);
    assert(dup[4] == '\0');
    _cc_free(dup);
    printf("strndupA test passed!\n");
}

void test_strndupW() {
    const wchar_t *str = L"test string";
    wchar_t *dup = _cc_strndupW(str, 4);
    assert(dup != NULL);
    assert(wcsncmp(str, dup, 4) == 0);
    assert(dup[4] == L'\0');
    _cc_free(dup);
    printf("strndupW test passed!\n");
}

int main() {
    test_malloc_and_free();
    test_calloc_and_free();
    test_realloc_and_free();
    test_malloc_zero_size();
    test_realloc_zero_size();
    test_free_nullptr();
    test_strdupA();
    test_strdupW();
    test_strndupA();
    test_strndupW();
    return 0;
}