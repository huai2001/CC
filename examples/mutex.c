
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libcc.h>
_cc_mutex_t *mutex;

int32_t func(_cc_thread_t *thrd, void* arg) {
    char* str = (char*)arg;
    if(*(char*)arg == '\0') return 0;
    _cc_mutex_lock(mutex);
    while(*str != '\0'){
        fputc(*str, stdout);
        str++;
    }
    fputc('\n', stdout);
    func(thrd, (char*)arg+1);
    _cc_mutex_unlock(mutex);
    return 0;
}

int main(){
    char str1[8], str2[8];
    _cc_thread_t *p1,*p2;
    mutex = _cc_alloc_mutex();
    
    sprintf(str1, "abcdefg");
    sprintf(str2, "1234567");
    
    p1 = _cc_thread(func, "test 1", str1);
    if(!p1){
        exit(1);
    }
    p2 = _cc_thread(func, "test 2", str2);
    if(!p2){
        exit(1);
    }
    _cc_wait_thread(p1, nullptr);
    _cc_wait_thread(p2, nullptr);
    
    _cc_free_mutex(mutex);
}
