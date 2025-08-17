#include <stdio.h>

#include <libcc/thread.h>
#include <libcc/alloc.h>
#include <libcc/time.h>
#include <libcc/atomic.h>
#include <libcc/list.h>

char c = 0;

_cc_atomic32_t thread_count = 0;
_cc_atomic32_t print_index = 0;
_cc_semaphore_t *sem;
    /**/
void print_info(const tchar_t *fmt, ...) {
    tchar_t buff[1024];
    time_t t = time(nullptr);
    struct tm* local_time = localtime(&t);

    if ( nullptr != _tcschr( fmt, _T('%')) ) {
        va_list args;
        va_start(args, fmt);
        _vsntprintf(buff, 1024, fmt, args);
        va_end(args);

        fmt = buff;
    }

    _tprintf(_T("%0.4d-%0.2d-%0.2d %0.2d:%0.2d:%0.2d %s\n"), 
        local_time->tm_year + 1900,
        local_time->tm_mon + 1,
        local_time->tm_mday,
        local_time->tm_hour,
        local_time->tm_min,
        local_time->tm_sec, fmt);
}

int32_t fn_thread_print(_cc_thread_t *thrd, void* param) {
    _cc_atomic32_inc(&thread_count);

    while (c != 'q') {
        _cc_atomic32_t index = _cc_atomic32_inc(&print_index);;
        if (index >= 10) {
            _cc_atomic32_set(&print_index, 0);
        }
        _cc_semaphore_wait_timeout(sem, 10000);
        
        print_info(_T("printf:%d - threadID:%d"),index, _cc_get_thread_id(thrd));
    }

    printf("thread quit ID:%d\n", _cc_get_thread_id(thrd));
    _cc_atomic32_dec(&thread_count);

    return 1;
}
#define MAX_THREAD_COUNT 5
int main (int argc, char * const argv[]) {
    int i = 0;
    srand((uint32_t)time(nullptr));
    //_cc_install_memory_tracked();
    sem = _cc_alloc_semaphore(0);

    for (i = 0; i < MAX_THREAD_COUNT; i++) {
        _cc_thread_t *t = _cc_thread(fn_thread_print, "test print", nullptr);
        if (t) {
            _cc_detach_thread(t);
        }
    }

    while (c != 'q') {
        c = getchar();
        if (c == 's') {
            _cc_semaphore_post(sem);
            c = 0;
        }
    }
    
    while(thread_count > 0) {
        _cc_sleep(1);
    }
    
    //_cc_uninstall_memory_tracked();
    return 0;
}
