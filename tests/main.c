#include <stdio.h>
#include <libcc/libcc.h>



int main (int argc, char * const argv[]) {
	char c = 0;
    srand((uint32_t)time(NULL));
    _cc_alloc_async_event(0, NULL);

    while((c = getchar()) != 'q') {
        _cc_sleep(100);
    }

    _cc_free_async_event();
    return 0;
}