#ifndef BRIGHTS_DAEMON_H
#define BRIGHTS_DAEMON_H

#include <stdint.h>

typedef struct {
    const char *name;
    int (*init_func)(void);
    void (*main_loop)(void);
    void (*cleanup_func)(void);
    int auto_restart;
    uint64_t restart_delay_ms;
} daemon_info_t;

/* Daemon management functions */
int daemonize(void);
int daemon_register(const daemon_info_t *info);
int daemon_start_all(void);
int daemon_stop_all(void);
void daemon_main_loop(void);

/* Common daemon utilities */
int daemon_write_pidfile(const char *filename);
int daemon_remove_pidfile(const char *filename);
void daemon_signal_handler(int sig);

/* Predefined daemons */
extern const daemon_info_t syslogd_info;
extern const daemon_info_t dhcpd_info;
extern const daemon_info_t cron_info;

#endif