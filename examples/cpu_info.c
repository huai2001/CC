#include <stdio.h>
#include <cc/core/cpu_info.h>


int main (int argc, char * const argv[]) {
    char c = 0;

    _tprintf(_T("cpu sn is %s\n"), _cc_cpu_sn());
    _tprintf(_T("CPU Count: %d\n"), _cc_cpu_count());

    _tprintf(_T("\nPress the Q key to exit\n"));
    
    while (c != 'q') {
        c = getchar();
    }
    return 0;
}
