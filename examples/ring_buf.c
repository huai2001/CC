#include <stdio.h>
#include <libcc/variant.h>
#include <libcc/alloc.h>
#include <libcc/ring.h>
#include <libcc/time.h>
#include <libcc/thread.h>
#include <libcc/mutex.h>

int i = 0;
char c = 0;
int thread_count = 0;
_cc_thread_t *rw_thread[10];
_cc_mutex_t *rw_lock = nullptr;
_cc_ring_t *buf = nullptr;

typedef struct _test_
{
    uint32_t game_id;
    uint32_t group_id;
}_test_t;

void test_ring_print(_cc_ring_t *thiz, const tchar_t *info);

int32_t fn_thread_read(_cc_thread_t *thrd, void* param) {
    _test_t d = {0};
    while (rw_lock){
        _cc_mutex_lock(rw_lock);
        
        if(_cc_ring_pop(buf, (byte_t*)&d, sizeof(_test_t)) != 0) {
            _tprintf(_T("pop[%0.4d - %0.4d]\n"), d.game_id, d.group_id);
        }
        test_ring_print(buf, _T("fn thread read:"));
        _cc_mutex_unlock(rw_lock);
        _cc_sleep(1);
    }
    return 1;
}

int32_t fn_thread_write(_cc_thread_t *thrd, void* param) {
    _test_t p;
    while (rw_lock){
        _cc_mutex_lock(rw_lock);
        p.game_id = i;
        p.group_id = 1 + i;
        if(_cc_ring_push(buf, (byte_t*)&p, sizeof(_test_t))) {
            _tprintf(_T("push %d\n"), i++);
        }
        
        test_ring_print(buf, _T("fn thread write:"));
        _cc_mutex_unlock(rw_lock);
        _cc_sleep(1);
    }
    return 1;
}

int main (int argc, char * const argv[]) {
    _test_t d = {0};
    _cc_install_memory_tracked();
    buf = _cc_alloc_ring(5*sizeof(_test_t));

    for (i = 1; i < 20; i++) {
        d.game_id = i;
        d.group_id = 1 + i;
        if(_cc_ring_push(buf, (byte_t*)&d, sizeof(_test_t))) {
            _tprintf(_T("push %d\n"), i);
        } else {
            while(_cc_ring_pop(buf, (byte_t*)&d, sizeof(_test_t)) != 0) {
                _tprintf(_T("pop[%0.4d - %0.4d]\n"), d.game_id, d.group_id);
            }
        }
    }

    while(_cc_ring_pop(buf, (byte_t*)&d, sizeof(_test_t)) != 0) {
        _tprintf(_T("[%0.4d - %0.4d]\n"), d.game_id, d.group_id);
    }
    /*
    rw_lock = _cc_alloc_mutex();
    if (rw_lock == nullptr) {
        _tprintf(_T("create mutex lock fial.\n"));
        _cc_free_ring(buf);
        return 0;
    }
    rw_thread[thread_count] = _cc_thread(fn_thread_read, "read", nullptr);
    if(rw_thread[thread_count]) {
        thread_count++;
    }

    rw_thread[thread_count] = _cc_thread(fn_thread_write, "write 1", nullptr);
    if(rw_thread[thread_count]) {
        thread_count++;
    }
    rw_thread[thread_count] = _cc_thread(fn_thread_write, "write 2", nullptr);
    if(rw_thread[thread_count]) {
        thread_count++;
    }
    rw_thread[thread_count] = _cc_thread(fn_thread_write, "write 3", nullptr);
    if(rw_thread[thread_count]) {
        thread_count++;
    }
    
    while((c = getchar()) != 'q') {
        _cc_sleep(100);
    }

    for(i = 0; i < thread_count; i++) {
        _cc_wait_thread(rw_thread[i]);
    }

    if (rw_lock) {
        _cc_free_mutex(rw_lock);
    }
    */
    _tprintf(_T("\n"));
    _cc_free_ring(buf);
    _cc_uninstall_memory_tracked();
    return 0;
}
