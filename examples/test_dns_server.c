#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libcc.h>

#include <libcc/widgets/widgets.h>

int main (int argc, char* argv[]) {
    _cc_install_async_event(0, nullptr);

    _cc_dns_listen();
    while (getchar() != 'q') {
        _cc_sleep(100);
    }
    _cc_uninstall_async_event();
    return 0;
}

