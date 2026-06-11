#ifndef BRIGHTS_INIT_SERVICE_H
#define BRIGHTS_INIT_SERVICE_H

#include <stdint.h>

#define MAX_SERVICES 32
#define SERVICE_NAME_MAX 32
#define SERVICE_DESC_MAX 128
#define SERVICE_CMD_MAX 256

typedef enum {
    SERVICE_STOPPED = 0,
    SERVICE_STARTING = 1,
    SERVICE_RUNNING = 2,
    SERVICE_STOPPING = 3,
    SERVICE_FAILED = 4,
    SERVICE_RESTARTING = 5
} service_state_t;

typedef enum {
    SERVICE_TYPE_DAEMON = 0,    /* Background daemon */
    SERVICE_TYPE_ONESHOT = 1,   /* Run once and exit */
    SERVICE_TYPE_FORKING = 2,   /* Forking service */
    SERVICE_TYPE_NOTIFY = 3     /* Notify when ready */
} service_type_t;

typedef struct {
    char name[SERVICE_NAME_MAX];
    char description[SERVICE_DESC_MAX];
    char command[SERVICE_CMD_MAX];
    char *argv[16];             /* Command arguments */
    char *envp[16];             /* Environment variables */
    service_type_t type;
    service_state_t state;
    int64_t pid;               /* Process ID when running */
    int restart;                /* Auto-restart on failure */
    int restart_delay;          /* Delay before restart (ms) */
    int64_t start_time;         /* When service was started */
    int64_t last_restart;       /* Last restart timestamp */
    int restart_count;          /* Number of restarts */
    char depends_on[8][SERVICE_NAME_MAX]; /* Dependencies */
    int depend_count;
} service_t;

/* Service management functions */
int service_init(void);
int service_load_config(const char *config_file);
int service_start(const char *name);
int service_stop(const char *name);
int service_restart(const char *name);
int service_status(const char *name, service_t *info);
int service_list(service_t *services, int max_count);
int service_reload_config(void);

/* Internal functions */
void service_monitor_loop(void);
void service_check_dependencies(const char *name);
void service_handle_sigchld(int64_t pid, int status);

#endif