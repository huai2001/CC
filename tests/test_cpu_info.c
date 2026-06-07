#include <assert.h>
#include <stdio.h>

#include <libcc/cores.h>
#include <libcc/time.h>

#ifdef _CC_HAVE_SYSCONF_
#include <unistd.h>
#endif

int main() {
    int cpu_cores = _cc_get_cpu_cores();
    double cpu_usage = _cc_get_cpu_usage();
#if defined(_SC_NPROCESSORS_ONLN)
    printf("SC_NPROCESSORS_ONLN: %d\n", cpu_cores);
#endif

#ifdef __CC_WINDOWS__
    printf("Windows: %d\n", cpu_cores);
#endif

#if defined(__CC_MACOSX__) || defined(__CC_IPHONEOS__)
    printf("MacOSX: %d\n", cpu_cores);
#endif
#ifdef __CC_OS2__
    printf("OS/2: %d\n", cpu_cores);
#endif
    if (cpu_usage <= 0) {
        _cc_sleep(1000);
        cpu_usage = _cc_get_cpu_usage();
    }
    printf("All tests passed! cpu cores:%d, cpu usage: %lf\n", cpu_cores, cpu_usage);

    double total = 0.0f, used = 0.0f;

    _cc_get_memory_usage(&total, &used);
    printf("Memory Total %lf G, Used:%lf G", total/1024.0, used/1024.0);
    return 0;
}