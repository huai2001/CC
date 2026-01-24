#include <libcc/array.h>
#include <assert.h>
#include <stdio.h>

void test_alloc_array() {
    _cc_array_t array = _cc_alloc_array(10);
    assert(_cc_array_available(array) == 10);
    assert(_cc_array_length(array) == 0);
    _cc_free_array(array);
    printf("Alloc array test passed!\n");
}

void test_realloc_array() {
    _cc_array_t array = _cc_alloc_array(10);
    assert(_cc_array_available(array) == 10);

    array = _cc_realloc_array(array, 20);
    assert(_cc_array_available(array) == 20);
    assert(_cc_array_length(array) == 0);

    _cc_free_array(array);

    printf("Realloc array test passed!\n");
}

void test_array_get() {
    int i;
    _cc_array_t array = _cc_alloc_array(10);

    for (i = 0; i < 10; i++) {
        _cc_array_push(&array, i + 1);
    }

    assert(_cc_array_get(array, 0) == 1);
    assert(_cc_array_get(array, 11) == -1);
    _cc_free_array(array);
    printf("Array find test passed!\n");
}

void test_array_push() {
    int i;
    _cc_array_t array = _cc_alloc_array(1);
    size_t index = _cc_array_push(&array, 1);
    assert(_cc_array_length(array) == 1);
    assert(index == 0);
    for (i = 0; i < 10; i++) {
        _cc_array_push(&array, i + 100);
    }
    for (i = 0; i < 10; i++) {  
        printf("%d,", (int)*((uintptr_t*)(array) + i));
    }
    putchar('\n');

    assert(_cc_array_get(array, 0) == 1);
    assert(_cc_array_get(array, 1) == 100);
    _cc_free_array(array);
    printf("Array push test passed!\n");
}

void test_array_pop() {
    _cc_array_t array = _cc_alloc_array(10);
    _cc_array_push(&array, 1);
    assert(_cc_array_pop(array) == 1);
    assert(_cc_array_length(array) == 0);
    _cc_free_array(array);
    printf("Array pop test passed!\n");
}

void test_array_append() {
    int i;
    size_t length;
    _cc_array_t array1 = _cc_alloc_array(10);
    _cc_array_t array2 = _cc_alloc_array(64);

    for (i = 0; i < 10; i++) {
        _cc_array_push(&array1, i);
    }

    for (i = 0; i < 64; i++) {
        _cc_array_push(&array2, i + 100);
    }
    _cc_array_append(&array1, array2);

    length = _cc_array_length(array1);
    for (i = 0; i < length; i++) {  
        printf("%d,", (int)*((uintptr_t*)(array1) + i));
    }
    putchar('\n');
    assert(length == 74);
    assert(_cc_array_get(array1, 1) == 1);
    assert(_cc_array_get(array1, 10) == 100);

    _cc_free_array(array1);
    _cc_free_array(array2);
    printf("Array append test passed!\n");
}

void test_array_set() {
    int i;
    _cc_array_t array = _cc_alloc_array(10);
    for (i = 0; i < 10; i++) {
        _cc_array_push(&array, i);
    }
    _cc_array_set(array, 0, 100);

    assert(_cc_array_get(array, 0) == 100);
    assert(_cc_array_available(array) == 0);
    assert(_cc_array_length(array) == 10);

    _cc_free_array(array);

    printf("Array set test passed!\n");
}

void test_array_remove() {
    int value;
    int i;
    _cc_array_t array = _cc_alloc_array(10);
    for (i = 0; i < 10; i++) {
        _cc_array_push(&array, i);
    }
    value = _cc_array_remove(array, 8);
    
    assert(value == 8);
    assert(_cc_array_available(array) == 1);
    assert(_cc_array_length(array) == 9);

    _cc_free_array(array);
    printf("Array remove test passed!\n");
}

void test_array_cleanup() {
    int i;
    _cc_array_t array = _cc_alloc_array(10);
    for (i = 0; i < 10; i++) {
        _cc_array_push(&array, i);
    }

    assert(_cc_array_available(array) == 0);
    assert(_cc_array_length(array) == 10);

    _cc_array_clear(array);

    assert(_cc_array_available(array) == 10);
    assert(_cc_array_length(array) == 0);

    _cc_free_array(array);
    printf("Array cleanup test passed!\n");
}

void test_array_begin_end() {
    int i;
    _cc_array_t array = _cc_alloc_array(10);
    for (i = 0; i < 10; i++) {
        _cc_array_push(&array, i);
    }
    assert(*_cc_array_begin(array) == 0);
    assert(*_cc_array_end(array) == 9);

    _cc_free_array(array);
    printf("Array begin/end test passed!\n");
}

int main() {
    test_alloc_array();
    test_realloc_array();
    test_array_get();
    test_array_push();
    test_array_pop();
    test_array_append();
    test_array_remove();
    test_array_cleanup();
    test_array_begin_end();
    printf("All array tests passed!\n");
    return 0;
}