#include <libcc/hmap.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Helper function to compare two uintptr_t values */
static bool_t equals_func(uintptr_t a, uintptr_t b) {
    return a == b;
}

/* Helper function to generate a hash for uintptr_t */
static intptr_t hash_func(uintptr_t key) {
    return (intptr_t)key;
}

void test_hmap_alloc_and_free() {
    _cc_hmap_t hmap;
    assert(_cc_alloc_hmap(&hmap, 10, equals_func, hash_func) == true);
    assert(hmap.limit >= 10);
    assert(hmap.slots != NULL);
    assert(_cc_free_hmap(&hmap) == true);
    printf("hmap alloc and free test passed!\n");
}

void test_hmap_push_and_find() {
    _cc_hmap_t hmap;
    assert(_cc_alloc_hmap(&hmap, 10, equals_func, hash_func) == true);
    
    uintptr_t key = 123;
    uintptr_t value = 456;
    
    assert(_cc_hmap_push(&hmap, key, value) == true);
    assert(_cc_hmap_find(&hmap, key) == value);
    
    assert(_cc_free_hmap(&hmap) == true);
    printf("hmap push and find test passed!\n");
}

void test_hmap_push_duplicate() {
    _cc_hmap_t hmap;
    assert(_cc_alloc_hmap(&hmap, 10, equals_func, hash_func) == true);
    
    uintptr_t key = 123;
    uintptr_t value1 = 456;
    uintptr_t value2 = 789;
    
    assert(_cc_hmap_push(&hmap, key, value1) == true);
    assert(_cc_hmap_push(&hmap, key, value2) == false);
    assert(_cc_hmap_find(&hmap, key) == value1);
    
    assert(_cc_free_hmap(&hmap) == true);
    printf("hmap push duplicate test passed!\n");
}

void test_hmap_pop() {
    _cc_hmap_t hmap;
    assert(_cc_alloc_hmap(&hmap, 10, equals_func, hash_func) == true);
    
    uintptr_t key = 123;
    uintptr_t value = 456;
    
    assert(_cc_hmap_push(&hmap, key, value) == true);
    assert(_cc_hmap_pop(&hmap, key) == value);
    assert(_cc_hmap_find(&hmap, key) == 0);
    
    assert(_cc_free_hmap(&hmap) == true);
    printf("hmap pop test passed!\n");
}

void test_hmap_cleanup() {
    _cc_hmap_t hmap;
    assert(_cc_alloc_hmap(&hmap, 10, equals_func, hash_func) == true);
    
    uintptr_t key1 = 123;
    uintptr_t value1 = 456;
    uintptr_t key2 = 789;
    uintptr_t value2 = 101112;
    
    assert(_cc_hmap_push(&hmap, key1, value1) == true);
    assert(_cc_hmap_push(&hmap, key2, value2) == true);
    
    assert(_cc_hmap_cleanup(&hmap) == true);
    assert(hmap.count == 0);
    assert(_cc_hmap_find(&hmap, key1) == 0);
    assert(_cc_hmap_find(&hmap, key2) == 0);
    
    assert(_cc_free_hmap(&hmap) == true);
    printf("hmap cleanup test passed!\n");
}

void test_hmap_rehash() {
    _cc_hmap_t hmap;
    assert(_cc_alloc_hmap(&hmap, 2, equals_func, hash_func) == true);
    
    uintptr_t key1 = 123;
    uintptr_t value1 = 456;
    uintptr_t key2 = 789;
    uintptr_t value2 = 101112;
    uintptr_t key3 = 131415;
    uintptr_t value3 = 161718;
    
    assert(_cc_hmap_push(&hmap, key1, value1) == true);
    assert(_cc_hmap_push(&hmap, key2, value2) == true);
    assert(_cc_hmap_push(&hmap, key3, value3) == true);
    
    assert(hmap.limit > 2);
    assert(_cc_hmap_find(&hmap, key1) == value1);
    assert(_cc_hmap_find(&hmap, key2) == value2);
    assert(_cc_hmap_find(&hmap, key3) == value3);
    
    assert(_cc_free_hmap(&hmap) == true);
    printf("hmap rehash test passed!\n");
}

int main() {
    test_hmap_alloc_and_free();
    test_hmap_push_and_find();
    test_hmap_push_duplicate();
    test_hmap_pop();
    test_hmap_cleanup();
    test_hmap_rehash();
    
    printf("All hmap tests passed!\n");
    return 0;
}