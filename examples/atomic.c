#include <stdio.h>

#include <cc/thread.h>
#include <cc/alloc.h>
#include <cc/time.h>
#include <cc/list.h>
#include <cc/atomic.h>

char c = 0;
_cc_atomic32_t g_int = 0;
_cc_atomic32_t g_test32 = 0;
_cc_atomic64_t g_test64 = 0;
#define THREAD_COUNT 5

int32_t fn_thread(_cc_thread_t *thrd, void* param) {
    int i = 0;
    while(1) {
        _cc_atomic32_inc(&g_int);
        _cc_sleep(10);
        if(++i >= 10)
            break;
    }
    return 1;
}

int main (int argc, char * const argv[]) {
    int i = 0;
    
    _cc_atomic32_t r32 = 0;
    _cc_atomic64_t r64 = 0;
    
    for (i = 0; i < THREAD_COUNT; i++) {
        _cc_thread_start(fn_thread,NULL,NULL);
    }

    printf("platform name:%s\n", _CC_PLATFORM_NAME_);

	r32 = _cc_atomic32_inc(&g_test32);
	_tprintf("atomic32_inc:%d, %d\n", g_test32, r32);
    _cc_atomic32_cas(&g_test32, 1, 4);
    _tprintf("atomic32_cas:%d, %d\n", g_test32, r32);
	r32 = _cc_atomic32_dec(&g_test32);
	_tprintf("atomic32_dec:%d, %d\n", g_test32, r32);


	r32 = _cc_atomic32_add(&g_test32, 1);
	_tprintf("atomic32_add:%d, %d\n", g_test32, r32);
	r32 = _cc_atomic32_sub(&g_test32, 1);
	_tprintf("atomic32_sub:%d, %d\n", g_test32, r32);

	r64 = _cc_atomic64_inc(&g_test64);
	_tprintf("atomic64_inc:%lld, %lld\n", g_test64, r64);
    _cc_atomic64_cas(&g_test64, 1, 4);
    _tprintf("atomic64_cas:%lld, %lld\n", g_test64, r64);
	r64 = _cc_atomic64_dec(&g_test64);
	_tprintf("atomic64_dec:%lld, %lld\n", g_test64, r64);

	r64 = _cc_atomic64_add(&g_test64,1);
	_tprintf("atomic64_add:%lld, %lld\n", g_test64, r64);
	r64 = _cc_atomic64_sub(&g_test64,1);
	_tprintf("atomic64_sub:%lld, %lld\n", g_test64, r64);

    while (c != 'q') {
        c = getchar();
        if(c == 'p') {
            printf("%d\n", g_int);
        }
    }
    return 0;
}
