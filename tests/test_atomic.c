#include <libcc/atomic.h>
#include <assert.h>
#include <stdio.h>

void test_atomic32_load() {
    _cc_atomic32_t a = 42;
    assert(_cc_atomic32_load(&a) == 42);
}

/**
 * @brief 测试原子64位整数的获取操作。
 *
 * 该函数用于验证 `_cc_atomic64_load` 的功能，确保其能够正确读取原子64位整数的值。
 * 测试中初始化一个原子变量为42，并通过断言检查 `_cc_atomic64_load` 的返回值是否为42。
 */
void test_atomic64_load() {
    _cc_atomic64_t a = 42;
    assert(_cc_atomic64_load(&a) == 42);
}

void test_atomic32_add() {
    _cc_atomic32_t a = 10;
    assert(_cc_atomic32_add(&a, 5) == 10);
    assert(a == 15);
}

void test_atomic64_add() {
    _cc_atomic64_t a = 10;
    assert(_cc_atomic64_add(&a, 5) == 10);
    assert(a == 15);
}

void test_atomic32_sub() {
    _cc_atomic32_t a = 10;
    assert(_cc_atomic32_sub(&a, 5) == 10);
    assert(a == 5);
}

void test_atomic64_sub() {
    _cc_atomic64_t a = 10;
    assert(_cc_atomic64_sub(&a, 5) == 10);
    assert(a == 5);
}

void test_atomic32_set() {
    _cc_atomic32_t a = 10;
    assert(_cc_atomic32_set(&a, 20) == 10);
    assert(a == 20);
}

void test_atomic64_set() {
    _cc_atomic64_t a = 10;
    assert(_cc_atomic64_set(&a, 20) == 10);
    assert(a == 20);
}

void test_atomic32_and() {
    _cc_atomic32_t a = 0xF0;
    assert(_cc_atomic32_and(&a, 0x0F) == 0xF0);
    assert(a == 0x00);
}

void test_atomic64_and() {
    _cc_atomic64_t a = 0xF0;
    assert(_cc_atomic64_and(&a, 0x0F) == 0xF0);
    assert(a == 0x00);
}

void test_atomic32_or() {
    _cc_atomic32_t a = 0xF0;
    assert(_cc_atomic32_or(&a, 0x0F) == 0xF0);
    assert(a == 0xFF);
}

void test_atomic64_or() {
    _cc_atomic64_t a = 0xF0;
    assert(_cc_atomic64_or(&a, 0x0F) == 0xF0);
    assert(a == 0xFF);
}

void test_atomic32_xor() {
    _cc_atomic32_t a = 0xF0;
    assert(_cc_atomic32_xor(&a, 0x0F) == 0xF0);
    assert(a == 0xFF);
}

void test_atomic64_xor() {
    _cc_atomic64_t a = 0xF0;
    assert(_cc_atomic64_xor(&a, 0x0F) == 0xF0);
    assert(a == 0xFF);
}

void test_atomic32_cas() {
    _cc_atomic32_t a = 10;
    assert(_cc_atomic32_cas(&a, 10, 20) == true);
    assert(a == 20);
    assert(_cc_atomic32_cas(&a, 10, 30) == false);
    assert(a == 20);
}

void test_atomic64_cas() {
    _cc_atomic64_t a = 10;
    assert(_cc_atomic64_cas(&a, 10, 20) == true);
    assert(a == 20);
    assert(_cc_atomic64_cas(&a, 10, 30) == false);
    assert(a == 20);
}

int main() {
    #if __CC_STDC_VERSION__ >= 11
        printf("Use C++11 standard\n");
    #elif defined(__CC_MSVC__)
        printf("Use MSVC\n");
    #elif defined(__CC_GNUC__)
        printf("Use GCC/Clang\n");
    #else
        return __sync_fetch_and_add(a, v);
    #endif/*__CC_STDC_VERSION__ >= 11*/

    test_atomic32_load();
    test_atomic64_load();
    test_atomic32_add();
    test_atomic64_add();
    test_atomic32_sub();
    test_atomic64_sub();
    test_atomic32_set();
    test_atomic64_set();
    test_atomic32_and();
    test_atomic64_and();
    test_atomic32_or();
    test_atomic64_or();
    test_atomic32_xor();
    test_atomic64_xor();
    test_atomic32_cas();
    test_atomic64_cas();

    printf("All atomic tests passed!\n");
    return 0;
}