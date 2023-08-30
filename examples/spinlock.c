#include <stdio.h>
#include <libcc.h>

#define USE_MUTEX 0

#define _COUNT (1024*1024*10)

#if USE_MUTEX
_cc_mutex_t* mutexlock;
#else
_cc_spinlock_t spinlock;
#endif
clock_t start, end;
int32_t refcount = 0;
int32_t func(_cc_thread_t *thrd, void* arg) {
    int index;
    for (index=0; index<_COUNT; index+=1) {
#if USE_MUTEX
    _cc_mutex_lock(mutexlock);
    refcount++;
    _cc_mutex_unlock(mutexlock);
#else
    _cc_spin_lock(&spinlock);
    refcount++;
    _cc_spin_unlock(&spinlock);
#endif
    }
    return 0;
}

int main (int argc, char * const argv[]) {
    _cc_thread_t *p1,*p2;
#if USE_MUTEX
    mutexlock = _cc_create_mutex();
#else
    _cc_spin_lock_init(&spinlock);
#endif
    
    start = clock();
    
    p1 = _cc_create_thread(func, "test 1", NULL);
    if(!p1){
        exit(1);
    }
    
    p2 = _cc_create_thread(func, "test 2", NULL);
    if(!p2){
        exit(1);
    }
    
    _cc_wait_thread(p1, NULL);
    _cc_wait_thread(p2, NULL);
    
    end = clock();
    printf("lock time span: %f seconds refcount:%d\n", (double)(end - start) / CLOCKS_PER_SEC, refcount);

    return 0;
}
