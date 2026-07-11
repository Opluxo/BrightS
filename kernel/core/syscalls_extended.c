#include <stdint.h>
#include <stddef.h>
#include "kernel_util.h"
#include "monitor.h"
#include "simd.h"
#include "proc.h"
#include "kmalloc.h"
#include "pmem.h"
#include "clock.h"

/* System call: sys_monitor_get_stats */
int64_t sys_monitor_get_stats(void *stats_buf, size_t buf_size)
{
    if (!stats_buf || buf_size < sizeof(performance_stats_t)) {
        return -1;
    }

    performance_stats_t stats;
    if (brights_monitor_get_performance_stats(&stats) != 0) {
        return -1;
    }

    if (kutil_memcpy(stats_buf, &stats, sizeof(performance_stats_t)) != stats_buf) {
        return -1;
    }

    return 0;
}

/* System call: sys_monitor_get_health */
int64_t sys_monitor_get_health(void *health_buf, size_t buf_size)
{
    if (!health_buf || buf_size < sizeof(system_health_t)) {
        return -1;
    }

    const system_health_t *health = brights_monitor_get_health();
    if (!health) {
        return -1;
    }

    if (kutil_memcpy(health_buf, health, sizeof(system_health_t)) != health_buf) {
        return -1;
    }

    return 0;
}

/* System call: sys_simd_available */
int64_t sys_simd_available(void)
{
    return (brights_simd_caps.has_sse2 ? 1 : 0) |
           (brights_simd_caps.has_avx ? 2 : 0) |
           (brights_simd_caps.has_avx2 ? 4 : 0);
}

/* System call: sys_simd_memcpy */
int64_t sys_simd_memcpy(void *dst, const void *src, size_t n)
{
    if (!dst || !src) {
        return -1;
    }

    brights_simd_memcpy(dst, src, n);
    return 0;
}

/* System call: sys_get_system_info */
int64_t sys_get_system_info(void *info_buf, size_t buf_size)
{
    if (!info_buf || buf_size < 256) {
        return -1;
    }

    char *buf = (char *)info_buf;

    kutil_strcpy(buf, "BrightS Operating System\n");
    buf += kutil_strlen(buf);

    kutil_strcpy(buf, "CPU: BrightS Virtual CPU\n");
    buf += kutil_strlen(buf);

    kutil_strcpy(buf, "Memory: available\n");
    buf += kutil_strlen(buf);

    kutil_strcpy(buf, "Disk: available\n");
    buf += kutil_strlen(buf);

    kutil_strcpy(buf, "Network: Ethernet\n");
    buf += kutil_strlen(buf);

    kutil_strcpy(buf, "Kernel: BrightS\n");
    buf += kutil_strlen(buf);

    return 0;
}

/* System call: sys_process_list */
int64_t sys_process_list(void *proc_buf, size_t buf_size, int *count_out)
{
    if (!proc_buf || !count_out || buf_size < 1024) {
        return -1;
    }

    char *buf = (char *)proc_buf;

    kutil_strcpy(buf, "PID\tPPID\tSTATE\tNAME\n");
    buf += kutil_strlen(buf);

    kutil_strcpy(buf, "1\t0\trunning\tinit\n");
    buf += kutil_strlen(buf);

    kutil_strcpy(buf, "2\t1\trunning\tshell\n");
    buf += kutil_strlen(buf);

    *count_out = 2;
    return 0;
}

/* System call: sys_system_load */
int64_t sys_system_load(int64_t *load1, int64_t *load5, int64_t *load15)
{
    if (!load1 || !load5 || !load15) {
        return -1;
    }

    *load1 = 5;
    *load5 = 3;
    *load15 = 2;

    return 0;
}

/* System call: sys_disk_usage */
int64_t sys_disk_usage(const char *path, uint64_t *total, uint64_t *used, uint64_t *avail)
{
    if (!path || !total || !used || !avail) {
        return -1;
    }

    *total = 50ULL * 1024 * 1024 * 1024;
    *used = 10ULL * 1024 * 1024 * 1024;
    *avail = *total - *used;

    return 0;
}

/* System call: sys_network_stats */
int64_t sys_network_stats(uint64_t *rx_bytes, uint64_t *tx_bytes, uint64_t *rx_packets, uint64_t *tx_packets)
{
    if (!rx_bytes || !tx_bytes || !rx_packets || !tx_packets) {
        return -1;
    }

    *rx_bytes = 0;
    *tx_bytes = 0;
    *rx_packets = 0;
    *tx_packets = 0;

    return 0;
}
