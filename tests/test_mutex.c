#include <libcc/mutex.h>
#include <libcc/thread.h>
#include <assert.h>
#include <stdio.h>

#define NUM_THREADS 4
#define NUM_ITERATIONS 1000

_cc_mutex_t *mutex;
int shared_counter = 0;

int thread_func(void* arg) {
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        _cc_mutex_lock(mutex);
        shared_counter++;
        _cc_mutex_unlock(mutex);
    }
    return 0;
}

void test_mutex_concurrent_access() {
    mutex = _cc_alloc_mutex();
    assert(mutex != NULL);

    _cc_thread_t *threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        threads[i] = _cc_thread(thread_func, "test_thread", NULL);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        _cc_wait_thread(threads[i], NULL);
    }

    assert(shared_counter == NUM_THREADS * NUM_ITERATIONS);
    _cc_free_mutex(mutex);
    printf("Concurrent access test passed!\n");
}

void test_mutex_try_lock() {
    mutex = _cc_alloc_mutex();
    assert(mutex != NULL);

    int result = _cc_mutex_try_lock(mutex);
    assert(result == 0);
    _cc_mutex_unlock(mutex);
    _cc_free_mutex(mutex);
    printf("Try lock test passed!\n");
}

void test_mutex_single_thread() {
    mutex = _cc_alloc_mutex();
    assert(mutex != NULL);

    _cc_mutex_lock(mutex);
    _cc_mutex_unlock(mutex);
    _cc_free_mutex(mutex);
    printf("Single thread test passed!\n");
}
int main() {
    test_mutex_single_thread();
    test_mutex_try_lock();
    test_mutex_concurrent_access();

    printf("All mutex tests passed!\n");
    return 0;
}