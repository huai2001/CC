#include <libcc/os.h>
#include <libcc/string.h>

#ifdef _CC_HAVE_SYSCONF_
#include <unistd.h>
#endif

#if defined(__CC_MACOSX__) || defined(__CC_IPHONEOS__)
#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/task_info.h>
#include <mach/mach_types.h>
#include <mach/mach.h>
#elif defined(__CC_OPENBSD__) && defined(__powerpc__)
#include <sys/types.h>
#include <sys/param.h>
#include <machine/cpu.h>
#elif defined(__CC_FREEBSD__) && defined(__powerpc__)
#include <machine/cpu.h>
#include <sys/auxv.h>
#endif

/**/
int _cc_cpu_cores = 0;

/**/
_CC_API_PUBLIC(int) _cc_get_cpu_cores(void) {
#if defined(_SC_NPROCESSORS_ONLN)
    if (_cc_cpu_cores <= 0) {
        _cc_cpu_cores = (int)sysconf(_SC_NPROCESSORS_ONLN);
    }
#endif

#ifdef __CC_WINDOWS__
    if (_cc_cpu_cores <= 0) {
        SYSTEM_INFO info;
        GetSystemInfo(&info);
        _cc_cpu_cores = info.dwNumberOfProcessors;
    }
#endif

#if defined(__CC_MACOSX__) || defined(__CC_IPHONEOS__)
    if (_cc_cpu_cores <= 0) {
        size_t size = sizeof(_cc_cpu_cores);
        sysctlbyname("hw.ncpu", &_cc_cpu_cores, &size, NULL, 0);
    }
#endif
#ifdef __CC_OS2__
    if (_cc_cpu_cores <= 0) {
        DosQuerySysInfo(QSV_NUMPROCESSORS, QSV_NUMPROCESSORS,
                        &_cc_cpu_cores, sizeof(_cc_cpu_cores) );
    }
#endif
    /* There has to be at least 1, right? :) */
    if (_cc_cpu_cores <= 0) {
        _cc_cpu_cores = 1;
    }
    return _cc_cpu_cores;
}
