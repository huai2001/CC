#include <stdio.h>
#include <libcc.h>

static int thread_logger(void *args) {
    int i = 0;
    int thread_id = _cc_get_thread_id(NULL);
    //while (1) {
    for (;i < 200;) {
        _cc_logger_debug("%s id:%d, i:%03f, i:%06u, i:0x%0.7x, %s :0x%0.8xAA", "test string", thread_id, (float)i*0.002f, i, i, "i << 8", i << 8);    
        i++;
    }
    return 1;
}

int main (int argc, char * const argv[]) {
	char c = 0;
    _cc_logger_dump();
    _cc_thread_start(thread_logger, "logger", NULL);

    while((c = getchar()) != 'q') {
        _cc_sleep(100);
    }
    return 0;
}