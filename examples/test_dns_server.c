#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libcc.h>

#include <cc/widgets/widgets.h>

int main (int argc, char* argv[]) {
    _cc_event_loop(0, NULL);

    _cc_dns_listen();
    while (getchar() != 'q') {
        _cc_sleep(100);
    }
    _cc_quit_event_loop();
    return 0;
}

