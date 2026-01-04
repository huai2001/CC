#import <Cocoa/Cocoa.h>
#import <sys/sysctl.h>

#include <mach/mach.h>
#include <libcc/logger.h>

_CC_API_PUBLIC(bool_t) _cc_open_url(const tchar_t *url) {
    @autoreleasepool {
        CFURLRef cfurl = CFURLCreateWithBytes(NULL, (const UInt8 *)url, _tcslen(url), kCFStringEncodingUTF8, NULL);
        OSStatus status = LSOpenCFURLRef(cfurl, NULL);
        CFRelease(cfurl);
        if (status != noErr) {
            _cc_logger_error(_T("LSOpenCFURLRef() failed: %d"), status);
            return false;
        }
        return true;
    }
}

_CC_API_PUBLIC(size_t) _cc_get_device_name(tchar_t *cname, size_t length) {
    sysctlbyname("kern.hostname", cname, &length, NULL, 0);
    return length;
}

/**
 * @brief Get the current CPU usage percentage of the system
 *
 * This function retrieves CPU statistics through Mach kernel APIs to calculate CPU usage.
 * It uses a time-difference-based algorithm to calculate instantaneous
 * CPU usage, avoiding directly returning raw counter values.
 *
 * @return double Returns a CPU usage percentage value between 0.0 and 100.0
 *              Returns 0.0 if system time cannot be obtained or on the first call
 *
 * @note This function uses static variables to store counter values from the previous call,
 *       so consecutive calls are required to obtain accurate usage data
 * @note The return value is calculated based on the time difference between two calls,
 *       the first call always returns 0.0
 */
_CC_API_PUBLIC(double) _cc_get_cpu_usage(void) {
    static unsigned long long lastTotal = 0, lastIdle = 0;
    host_cpu_load_info_data_t cpu_info;
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    unsigned long long total, diffTotal, diffIdle;

    if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, (host_info_t)&cpu_info, &count) != KERN_SUCCESS) {
        return 0.0;
    }

    /* Calculate total CPU time */
    total = (unsigned long long)(cpu_info.cpu_ticks[CPU_STATE_USER] +
                                cpu_info.cpu_ticks[CPU_STATE_SYSTEM] +
                                cpu_info.cpu_ticks[CPU_STATE_IDLE] +
                                cpu_info.cpu_ticks[CPU_STATE_NICE]);

    if (lastTotal > 0) {
        diffTotal = total - lastTotal;
        diffIdle = (unsigned long long)(cpu_info.cpu_ticks[CPU_STATE_IDLE] - lastIdle);

        if (diffTotal > 0) {
            lastTotal = total;
            lastIdle = cpu_info.cpu_ticks[CPU_STATE_IDLE];
            return (double)(diffTotal - diffIdle) / diffTotal * 100;
        }
    }

    lastTotal = total;
    lastIdle = cpu_info.cpu_ticks[CPU_STATE_IDLE];
    return 0.0;
}

/**
 * @brief Get system memory usage information
 *
 * @param[out] total Pointer to a double that will store the total memory in MB
 * @param[out] used Pointer to a double that will store the used memory in MB
 *
 * @note This function retrieves physical memory information through sysctl
 * @note The returned memory unit is MB (Megabytes)
 */
_CC_API_PUBLIC(void) _cc_get_memory_usage(double* total, double* used) {
    int mib[2];
    unsigned long long memTotal;
    int64_t page_size = 0;
    size_t length;

    if (total == NULL && used == NULL) {
        return;
    }

    /* Get total memory */
    if (total != NULL) {
        mib[0] = CTL_HW;
        mib[1] = HW_MEMSIZE;
        length = sizeof(memTotal);
        if (sysctl(mib, 2, &memTotal, &length, NULL, 0) == 0) {
            *total = (double)memTotal / 1024.0 / 1024.0;
        } else {
            *total = 0.0;
        }
    }

    /* Get used memory */
    if (used != NULL) {
        /* Get page size */
        mib[0] = CTL_HW;
        mib[1] = HW_PAGESIZE;
        length = sizeof(page_size);
        if (sysctl(mib, 2, &page_size, &length, NULL, 0) != 0) {
            *used = 0.0;
            return;
        }

        /* Get VM statistics using host_statistics64 */
        vm_statistics64_data_t vm_stat;
        mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;

        if (host_statistics64(mach_host_self(), HOST_VM_INFO64,
                             (host_info64_t)&vm_stat, &count) == KERN_SUCCESS) {
            /* Calculate free memory: free + inactive + speculative */
            unsigned long long free = (unsigned long long)vm_stat.free_count +
                                     (unsigned long long)vm_stat.inactive_count +
                                     (unsigned long long)vm_stat.speculative_count;

            /* Get total memory again for calculation */
            mib[0] = CTL_HW;
            mib[1] = HW_MEMSIZE;
            length = sizeof(memTotal);
            if (sysctl(mib, 2, &memTotal, &length, NULL, 0) == 0) {
                unsigned long long used_mem = memTotal - (free * page_size);
                *used = (double)used_mem / 1024.0 / 1024.0;
            } else {
                *used = 0.0;
            }
        } else {
            *used = 0.0;
        }
    }
}
