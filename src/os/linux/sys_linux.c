#include <libcc/logger.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sysinfo.h>

/**
 * @brief Get the current CPU usage percentage of the system
 *
 * This function reads CPU statistics from /proc/stat to calculate CPU usage.
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
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
    unsigned long long total, diffTotal, diffIdle;
    FILE *fp;

    fp = fopen("/proc/stat", "r");
    if (fp == NULL) {
        return 0.0;
    }

    if (fscanf(fp, "cpu %llu %llu %llu %llu %llu %llu %llu",
                &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal) != 8) {
        fclose(fp);
        return 0.0;
    }
    fclose(fp);

    /* Calculate total CPU time */
    total = user + nice + system + idle + iowait + irq + softirq + steal;

    if (lastTotal > 0) {
        diffTotal = total - lastTotal;
        diffIdle = idle - lastIdle;

        if (diffTotal > 0) {
            lastTotal = total;
            lastIdle = idle;
            return (double)(diffTotal - diffIdle) / diffTotal * 100;
        }
    }

    lastTotal = total;
    lastIdle = idle;
    return 0.0;
}

/**
 * @brief Get system memory usage information
 *
 * @param[out] total Pointer to a double that will store the total memory in MB
 * @param[out] used Pointer to a double that will store the used memory in MB
 *
 * @note This function retrieves physical memory information through sysinfo
 * @note The returned memory unit is MB (Megabytes)
 */
_CC_API_PUBLIC(void) _cc_get_memory_usage(double* total, double* used) {
    struct sysinfo info;

    if (sysinfo(&info) != 0) {
        if (total) *total = 0.0;
        if (used) *used = 0.0;
        return;
    }

    if (total) {
        *total = (double)info.totalram * info.mem_unit / 1024.0 / 1024.0;
    }

    if (used) {
        *used = (double)(info.totalram - info.freeram) * info.mem_unit / 1024.0 / 1024.0;
    }
}
