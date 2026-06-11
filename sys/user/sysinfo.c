#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>

/*
 * BrightS System Information Tool
 * Displays comprehensive system information
 */

#define SYS_GET_SYSTEM_INFO 300
#define SYS_PROCESS_LIST    301
#define SYS_SYSTEM_LOAD     302
#define SYS_DISK_USAGE      303
#define SYS_NETWORK_STATS   304
#define SYS_MONITOR_GET_STATS 305
#define SYS_MONITOR_GET_HEALTH 306

typedef struct {
    double avg_cpu_usage;
    double peak_memory_usage;
    uint64_t total_disk_reads;
    uint64_t total_disk_writes;
    uint64_t total_network_rx;
    uint64_t total_network_tx;
    uint32_t process_count;
    double system_load_avg;
} performance_stats_t;

typedef struct {
    int overall_health;
    int cpu_health;
    int memory_health;
    int disk_health;
    int network_health;
    uint32_t active_alerts;
    uint64_t last_update;
} system_health_t;

void print_header(const char *title) {
    printf("\033[1;34m%s\033[0m\n", title);
    printf("\033[1;34m%s\033[0m\n", "═══════════════════════════════════════════");
}

void print_system_info() {
    print_header("System Information");

    char info[1024];
    if (syscall(SYS_GET_SYSTEM_INFO, info, sizeof(info)) == 0) {
        printf("%s", info);
    } else {
        printf("Unable to retrieve system information\n");
    }
    printf("\n");
}

void print_cpu_info() {
    print_header("CPU Information");

    printf("Architecture: x86_64\n");
    printf("Cores: 4 (8 threads)\n");
    printf("Frequency: 2.0 GHz\n");
    printf("Cache L1: 32KB\n");
    printf("Cache L2: 256KB\n");
    printf("Cache L3: 8MB\n");
    printf("\n");
}

void print_memory_info() {
    print_header("Memory Information");

    printf("Total: 1024 MB\n");
    printf("Used: 256 MB\n");
    printf("Free: 768 MB\n");
    printf("Swap: 512 MB total, 0 MB used\n");
    printf("\n");
}

void print_disk_info() {
    print_header("Disk Information");

    uint64_t total, used, free;
    if (syscall(SYS_DISK_USAGE, "/", &total, &used, &free) == 0) {
        printf("Total: %llu GB\n", total / (1024*1024*1024));
        printf("Used: %llu GB\n", used / (1024*1024*1024));
        printf("Free: %llu GB\n", free / (1024*1024*1024));
        printf("Usage: %.1f%%\n", (double)used / total * 100);
    } else {
        printf("Unable to retrieve disk information\n");
    }
    printf("\n");
}

void print_network_info() {
    print_header("Network Information");

    uint64_t rx_bytes, tx_bytes, rx_packets, tx_packets;
    if (syscall(SYS_NETWORK_STATS, &rx_bytes, &tx_bytes, &rx_packets, &tx_packets) == 0) {
        printf("RX: %llu bytes (%llu packets)\n", rx_bytes, rx_packets);
        printf("TX: %llu bytes (%llu packets)\n", tx_bytes, tx_packets);
        printf("Interface: eth0\n");
        printf("IP Address: 10.0.2.15\n");
        printf("Gateway: 10.0.2.2\n");
    } else {
        printf("Unable to retrieve network information\n");
    }
    printf("\n");
}

void print_process_info() {
    print_header("Process Information");

    char proc_buf[2048];
    int count;
    if (syscall(SYS_PROCESS_LIST, proc_buf, sizeof(proc_buf), &count) == 0) {
        printf("%s", proc_buf);
        printf("Total processes: %d\n", count);
    } else {
        printf("Unable to retrieve process information\n");
    }
    printf("\n");
}

void print_performance_stats() {
    print_header("Performance Statistics");

    performance_stats_t stats;
    if (syscall(SYS_MONITOR_GET_STATS, &stats, sizeof(stats)) == 0) {
        printf("CPU Usage: %.1f%%\n", stats.avg_cpu_usage);
        printf("Peak Memory: %.1f%%\n", stats.peak_memory_usage);
        printf("Disk Reads: %llu\n", stats.total_disk_reads);
        printf("Disk Writes: %llu\n", stats.total_disk_writes);
        printf("Network RX: %llu bytes\n", stats.total_network_rx);
        printf("Network TX: %llu bytes\n", stats.total_network_tx);
        printf("Process Count: %u\n", stats.process_count);
        printf("System Load: %.2f\n", stats.system_load_avg);
    } else {
        printf("Unable to retrieve performance statistics\n");
    }
    printf("\n");
}

void print_system_health() {
    print_header("System Health");

    system_health_t health;
    if (syscall(SYS_MONITOR_GET_HEALTH, &health, sizeof(health)) == 0) {
        printf("Overall Health: %d%%\n", health.overall_health);
        printf("CPU Health: %d%%\n", health.cpu_health);
        printf("Memory Health: %d%%\n", health.memory_health);
        printf("Disk Health: %d%%\n", health.disk_health);
        printf("Network Health: %d%%\n", health.network_health);
        printf("Active Alerts: %u\n", health.active_alerts);
    } else {
        printf("Unable to retrieve system health information\n");
    }
    printf("\n");
}

void print_load_average() {
    print_header("System Load");

    double load1, load5, load15;
    if (syscall(SYS_SYSTEM_LOAD, &load1, &load5, &load15) == 0) {
        printf("1 minute: %.2f\n", load1);
        printf("5 minutes: %.2f\n", load5);
        printf("15 minutes: %.2f\n", load15);
    } else {
        printf("Unable to retrieve load average\n");
    }
    printf("\n");
}

void print_colors() {
    print_header("Color Palette");

    // Standard ANSI colors
    for (int i = 0; i < 8; i++) {
        printf("\033[4%dm   ", i);
    }
    printf("\033[0m\n");

    for (int i = 8; i < 16; i++) {
        printf("\033[4%dm   ", i);
    }
    printf("\033[0m\n\n");
}

void print_help() {
    printf("BrightS System Information Tool\n\n");
    printf("Displays comprehensive system information including:\n");
    printf("  - System details (OS, kernel, hardware)\n");
    printf("  - Performance statistics\n");
    printf("  - System health status\n");
    printf("  - Process information\n");
    printf("  - Network status\n");
    printf("  - Color palette\n\n");
    printf("Usage: sysinfo [OPTIONS]\n\n");
    printf("Options:\n");
    printf("  --help, -h     Show this help message\n");
    printf("  --short, -s    Show brief information\n");
    printf("  --all, -a      Show all information (default)\n");
    printf("  --cpu          Show only CPU information\n");
    printf("  --memory       Show only memory information\n");
    printf("  --disk         Show only disk information\n");
    printf("  --network      Show only network information\n");
    printf("  --performance  Show performance statistics\n");
    printf("  --health       Show system health\n");
    printf("  --processes    Show process information\n");
    printf("  --load         Show system load\n");
    printf("  --colors       Show color palette\n");
    printf("\n");
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            print_help();
            return 0;
        }

        // Handle specific options
        if (strcmp(argv[1], "--cpu") == 0) {
            print_cpu_info();
            return 0;
        } else if (strcmp(argv[1], "--memory") == 0) {
            print_memory_info();
            return 0;
        } else if (strcmp(argv[1], "--disk") == 0) {
            print_disk_info();
            return 0;
        } else if (strcmp(argv[1], "--network") == 0) {
            print_network_info();
            return 0;
        } else if (strcmp(argv[1], "--performance") == 0) {
            print_performance_stats();
            return 0;
        } else if (strcmp(argv[1], "--health") == 0) {
            print_system_health();
            return 0;
        } else if (strcmp(argv[1], "--processes") == 0) {
            print_process_info();
            return 0;
        } else if (strcmp(argv[1], "--load") == 0) {
            print_load_average();
            return 0;
        } else if (strcmp(argv[1], "--colors") == 0) {
            print_colors();
            return 0;
        } else if (strcmp(argv[1], "--short") == 0 || strcmp(argv[1], "-s") == 0) {
            print_system_info();
            print_cpu_info();
            print_memory_info();
            return 0;
        }
    }

    // Default: show all information
    print_system_info();
    print_cpu_info();
    print_memory_info();
    print_disk_info();
    print_network_info();
    print_process_info();
    print_performance_stats();
    print_system_health();
    print_load_average();
    print_colors();

    return 0;
}