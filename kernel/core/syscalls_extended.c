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

    kutil_memcpy(stats_buf, &stats, sizeof(performance_stats_t));
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

    kutil_memcpy(health_buf, health, sizeof(system_health_t));
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
        return -1;
    }

    char *buf = (char *)info_buf;

    /* Collect basic system information */
    kutil_strcpy(buf, "BrightS Operating System\n");
    buf += kutil_strlen(buf);

    /* CPU info */
    extern const brights_cpu_info_t *brights_hwinfo_cpu(void);
    const brights_cpu_info_t *cpu = brights_hwinfo_cpu();
    if (cpu) {
        kutil_strcpy(buf, "CPU: ");
        buf += kutil_strlen(buf);
        kutil_strcpy(buf, cpu->vendor);
        buf += kutil_strlen(buf);
        kutil_strcpy(buf, "\n");
        buf += kutil_strlen(buf);
    }

    /* Memory info */
    extern uint64_t brights_pmem_total_bytes(void);
    extern uint64_t brights_pmem_free_bytes(void);
    uint64_t total_mb = brights_pmem_total_bytes() / (1024 * 1024);
    uint64_t free_mb = brights_pmem_free_bytes() / (1024 * 1024);
    kutil_sprintf(buf, "Memory: %d MB total, %d MB free\n", (int)total_mb, (int)free_mb);
    buf += kutil_strlen(buf);

    /* Kernel info */
    kutil_strcpy(buf, "Kernel: BrightS v0.1.2.7\n");
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
    int pos = 0;

    kutil_strcpy(buf, "PID\tPPID\tSTATE\tNAME\n");
    pos += kutil_strlen(buf);

    extern brights_proc_info_t *brights_proc_table_ptr(void);
    brights_proc_info_t *table = brights_proc_table_ptr();
    if (!table) {
        *count_out = 0;
        return 0;
    }

    int count = 0;
    const char *state_names[] = {"unused", "runnable", "running", "sleeping", "zombie"};
    
    for (int i = 0; i < 64 && pos < (int)(buf_size - 128); i++) {
        if (table[i].state == 0) continue;  /* BRIGHTS_PROC_UNUSED */
        
        kutil_sprintf(buf + pos, "%d\t%d\t%s\t%s\n", 
            table[i].pid, table[i].ppid,
            state_names[table[i].state],
            table[i].name);
        pos += kutil_strlen(buf + pos);
        count++;
    }

    *count_out = count;
    return 0;
}

/* System call: sys_system_load
 * Uses fixed-point representation: load * 100 (e.g., 0.5 = 50)
 * Returns load averages scaled by 100 for 1min, 5min, 15min
 */
int64_t sys_system_load(uint32_t *load1, uint32_t *load5, uint32_t *load15)
{
    if (!load1 || !load5 || !load15) {
        return -1;
    }

    /* Calculate real load averages from run queue */
    extern uint32_t brights_proc_count(uint32_t state);
    extern uint64_t brights_sched_ticks(void);
    
    uint32_t running = brights_proc_count(2);  /* BRIGHTS_PROC_RUNNING */
    uint32_t runnable = brights_proc_count(1); /* BRIGHTS_PROC_RUNNABLE */
    uint32_t load = running + runnable;
    
    /* Scale by 100 for fixed-point (e.g., 1.5 = 150) */
    /* Simple exponential moving average approximation */
    static uint32_t avg1 = 0, avg5 = 0, avg15 = 0;
    static uint64_t last_tick = 0;
    
    uint64_t now = brights_sched_ticks();
    if (last_tick == 0) {
        avg1 = avg5 = avg15 = load * 100;
        last_tick = now;
    } else if (now > last_tick) {
        /* Decay factor: 1 - 1/interval */
        /* For 1min: factor = 1 - 1/60*100 = 98/100 */
        /* For 5min: factor = 1 - 1/300*100 = 9967/10000 */
        /* For 15min: factor = 1 - 1/900*100 = 9989/10000 */
        avg1 = (avg1 * 98 + load * 100 * 2) / 100;
        avg5 = (avg5 * 9967 + load * 100 * 33) / 10000;
        avg15 = (avg15 * 9989 + load * 100 * 11) / 10000;
        last_tick = now;
    }
    
    *load1 = avg1;
    *load5 = avg5;
    *load15 = avg15;

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