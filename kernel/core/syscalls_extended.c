/* System call: sys_monitor_get_stats */
int64_t sys_monitor_get_stats(void *stats_buf, size_t buf_size)
{
    if (!stats_buf || buf_size < sizeof(performance_stats_t)) {
        return -EINVAL;
    }

    performance_stats_t stats;
    if (brights_monitor_get_performance_stats(&stats) != 0) {
        return -EIO;
    }

    if (kernel_memcpy(stats_buf, &stats, sizeof(performance_stats_t)) != stats_buf) {
        return -EFAULT;
    }

    return 0;
}

/* System call: sys_monitor_get_health */
int64_t sys_monitor_get_health(void *health_buf, size_t buf_size)
{
    if (!health_buf || buf_size < sizeof(system_health_t)) {
        return -EINVAL;
    }

    const system_health_t *health = brights_monitor_get_health();
    if (!health) {
        return -EIO;
    }

    if (kernel_memcpy(health_buf, health, sizeof(system_health_t)) != health_buf) {
        return -EFAULT;
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
        return -EINVAL;
    }

    brights_simd_memcpy(dst, src, n);
    return 0;
}

/* System call: sys_get_system_info */
int64_t sys_get_system_info(void *info_buf, size_t buf_size)
{
    if (!info_buf || buf_size < 256) {
        return -EINVAL;
    }

    char *buf = (char *)info_buf;

    /* Collect basic system information */
    kernel_strcpy(buf, "BrightS Operating System\n");
    buf += kernel_strlen(buf);

    /* CPU info */
    kernel_strcpy(buf, "CPU: BrightS Virtual CPU @ 2.0GHz\n");
    buf += kernel_strlen(buf);

    /* Memory info */
    kernel_sprintf(buf, "Memory: %d MB available\n", 1024);
    buf += kernel_strlen(buf);

    /* Disk info */
    kernel_sprintf(buf, "Disk: %d GB available\n", 50);
    buf += kernel_strlen(buf);

    /* Network info */
    kernel_strcpy(buf, "Network: Ethernet (10.0.2.15)\n");
    buf += kernel_strlen(buf);

    /* Kernel info */
    kernel_strcpy(buf, "Kernel: BrightS v0.1.2.2\n");
    buf += kernel_strlen(buf);

    return 0;
}

/* System call: sys_process_list */
int64_t sys_process_list(void *proc_buf, size_t buf_size, int *count_out)
{
    if (!proc_buf || !count_out || buf_size < 1024) {
        return -EINVAL;
    }

    /* This is a simplified implementation */
    char *buf = (char *)proc_buf;

    kernel_strcpy(buf, "PID\tPPID\tSTATE\tNAME\n");
    buf += kernel_strlen(buf);

    kernel_strcpy(buf, "1\t0\trunning\tinit\n");
    buf += kernel_strlen(buf);

    kernel_strcpy(buf, "2\t1\trunning\tshell\n");
    buf += kernel_strlen(buf);

    *count_out = 2;
    return 0;
}

/* System call: sys_system_load */
int64_t sys_system_load(double *load1, double *load5, double *load15)
{
    if (!load1 || !load5 || !load15) {
        return -EINVAL;
    }

    /* Simplified load averages */
    *load1 = 0.5;
    *load5 = 0.3;
    *load15 = 0.2;

    return 0;
}

/* System call: sys_disk_usage */
int64_t sys_disk_usage(const char *path, uint64_t *total, uint64_t *used, uint64_t *free)
{
    if (!path || !total || !used || !free) {
        return -EINVAL;
    }

    /* Simplified disk usage */
    *total = 50LL * 1024 * 1024 * 1024;  /* 50GB */
    *used = 10LL * 1024 * 1024 * 1024;   /* 10GB */
    *free = *total - *used;

    return 0;
}

/* System call: sys_network_stats */
int64_t sys_network_stats(uint64_t *rx_bytes, uint64_t *tx_bytes, uint64_t *rx_packets, uint64_t *tx_packets)
{
    if (!rx_bytes || !tx_bytes || !rx_packets || !tx_packets) {
        return -EINVAL;
    }

    /* Simplified network statistics */
    *rx_bytes = 1024 * 1024;     /* 1MB received */
    *tx_bytes = 512 * 1024;      /* 512KB sent */
    *rx_packets = 1000;          /* 1000 packets received */
    *tx_packets = 500;           /* 500 packets sent */

    return 0;
}