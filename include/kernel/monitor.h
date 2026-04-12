#ifndef BRIGHTS_MONITOR_H
#define BRIGHTS_MONITOR_H

#include <stdint.h>

/*
 * BrightS System Monitoring and Analysis Framework
 *
 * Provides comprehensive system monitoring including:
 * - Performance metrics collection
 * - Resource usage tracking
 * - Health monitoring and alerting
 * - Real-time analytics
 */

#define MONITOR_MAX_METRICS 64
#define MONITOR_MAX_ALERTS 16

/* Metric types */
typedef enum {
    METRIC_CPU_USAGE,
    METRIC_MEMORY_USAGE,
    METRIC_DISK_IO,
    METRIC_NETWORK_IO,
    METRIC_PROCESS_COUNT,
    METRIC_SYSTEM_LOAD,
    METRIC_TEMPERATURE,
    METRIC_POWER_CONSUMPTION,
    METRIC_CUSTOM = 100
} metric_type_t;

/* Alert severity levels */
typedef enum {
    ALERT_INFO,
    ALERT_WARNING,
    ALERT_ERROR,
    ALERT_CRITICAL
} alert_severity_t;

/* Metric data structure */
typedef struct {
    metric_type_t type;
    char name[32];
    uint64_t timestamp;
    union {
        uint64_t u64_value;
        int64_t i64_value;
        double f64_value;
    } value;
    char unit[8];  /* e.g., "%", "MB", "ops/s" */
} metric_t;

/* Alert configuration */
typedef struct {
    alert_severity_t severity;
    char name[32];
    char description[128];
    metric_type_t metric_type;
    double threshold;
    int condition;  /* 0: below threshold, 1: above threshold */
} alert_config_t;

/* System health status */
typedef struct {
    int overall_health;  /* 0-100 percentage */
    int cpu_health;
    int memory_health;
    int disk_health;
    int network_health;
    uint32_t active_alerts;
    uint64_t last_update;
} system_health_t;

/* Monitoring configuration */
typedef struct {
    int enabled;
    uint32_t collection_interval_ms;
    uint32_t retention_period_sec;
    uint32_t max_metrics_stored;
    alert_config_t alerts[MONITOR_MAX_ALERTS];
    int num_alerts;
} monitor_config_t;

/* Global monitoring state */
extern monitor_config_t monitor_config;
extern system_health_t system_health;

/* Core monitoring API */
int brights_monitor_init(void);
int brights_monitor_shutdown(void);
int brights_monitor_start_collection(void);
int brights_monitor_stop_collection(void);

/* Metric collection */
int brights_monitor_collect_metric(metric_type_t type, const char *name, ...);
int brights_monitor_get_latest_metric(metric_type_t type, metric_t *metric);
int brights_monitor_get_metrics(metric_t *metrics, int max_count, metric_type_t filter);

/* Health monitoring */
const system_health_t *brights_monitor_get_health(void);
int brights_monitor_update_health(void);

/* Alert management */
int brights_monitor_add_alert(const alert_config_t *alert);
int brights_monitor_remove_alert(const char *name);
int brights_monitor_check_alerts(void);

/* Configuration */
int brights_monitor_set_config(const monitor_config_t *config);
const monitor_config_t *brights_monitor_get_config(void);

/* Performance analysis */
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

int brights_monitor_get_performance_stats(performance_stats_t *stats);
int brights_monitor_generate_report(char *buffer, size_t buffer_size);

/* Utility functions */
const char *brights_monitor_metric_type_name(metric_type_t type);
const char *brights_monitor_alert_severity_name(alert_severity_t severity);

#endif /* BRIGHTS_MONITOR_H */