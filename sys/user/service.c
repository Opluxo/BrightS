#include "service.h"
#include "libc.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

static service_t services[MAX_SERVICES];
static int service_count = 0;

/* Forward declarations */
static int service_find(const char *name);
static int service_start_internal(service_t *svc);
static int service_stop_internal(service_t *svc);
static void service_update_state(service_t *svc, service_state_t state);
static int service_check_ready(service_t *svc);

/*
 * Initialize service management system
 */
int service_init(void)
{
    memset(services, 0, sizeof(services));
    service_count = 0;
    printf("service: initialized\n");
    return 0;
}

/*
 * Load service configuration from file
 */
int service_load_config(const char *config_file)
{
    int fd = sys_open(config_file, 0); /* O_RDONLY */
    if (fd < 0) {
        printf("service: cannot open config %s\n", config_file);
        return -1;
    }

    /* Simple config format:
     * [service_name]
     * description=Description
     * command=/path/to/executable
     * type=daemon|oneshot|forking|notify
     * restart=yes|no
     * restart_delay=1000
     * depends_on=service1,service2
     */

    char buf[1024];
    int64_t n = sys_read(fd, buf, sizeof(buf) - 1);
    if (n <= 0) {
        sys_close(fd);
        return -1;
    }
    buf[n] = 0;
    sys_close(fd);

    /* Parse config (simplified) */
    char *line = buf;
    service_t *current_svc = NULL;

    while (*line) {
        char *end = strchr(line, '\n');
        if (end) *end = 0;

        if (line[0] == '[' && strchr(line, ']')) {
            /* New service section */
            char name[SERVICE_NAME_MAX];
            sscanf(line + 1, "%[^]]", name);

            if (service_count >= MAX_SERVICES) {
                printf("service: too many services\n");
                break;
            }

            current_svc = &services[service_count++];
            memset(current_svc, 0, sizeof(service_t));
            strcpy(current_svc->name, name);
            current_svc->state = SERVICE_STOPPED;
            current_svc->restart = 0;
            current_svc->restart_delay = 1000;
            current_svc->type = SERVICE_TYPE_DAEMON;

        } else if (current_svc && strncmp(line, "description=", 12) == 0) {
            strcpy(current_svc->description, line + 12);
        } else if (current_svc && strncmp(line, "command=", 8) == 0) {
            strcpy(current_svc->command, line + 8);
        } else if (current_svc && strncmp(line, "type=", 5) == 0) {
            if (strcmp(line + 5, "daemon") == 0) current_svc->type = SERVICE_TYPE_DAEMON;
            else if (strcmp(line + 5, "oneshot") == 0) current_svc->type = SERVICE_TYPE_ONESHOT;
            else if (strcmp(line + 5, "forking") == 0) current_svc->type = SERVICE_TYPE_FORKING;
            else if (strcmp(line + 5, "notify") == 0) current_svc->type = SERVICE_TYPE_NOTIFY;
        } else if (current_svc && strncmp(line, "restart=", 8) == 0) {
            current_svc->restart = (strcmp(line + 8, "yes") == 0);
        } else if (current_svc && strncmp(line, "restart_delay=", 14) == 0) {
            current_svc->restart_delay = atoi(line + 14);
        } else if (current_svc && strncmp(line, "depends_on=", 11) == 0) {
            /* Parse comma-separated list */
            char *deps = line + 11;
            char *token = strtok(deps, ",");
            while (token && current_svc->depend_count < 8) {
                while (*token == ' ') token++; /* Trim spaces */
                strcpy(current_svc->depends_on[current_svc->depend_count++], token);
                token = strtok(NULL, ",");
            }
        }

        if (end) line = end + 1;
        else break;
    }

    printf("service: loaded %d services\n", service_count);
    return 0;
}

/*
 * Start a service by name
 */
int service_start(const char *name)
{
    int idx = service_find(name);
    if (idx < 0) {
        printf("service: service '%s' not found\n", name);
        return -1;
    }

    service_t *svc = &services[idx];

    /* Check dependencies */
    service_check_dependencies(name);

    return service_start_internal(svc);
}

/*
 * Stop a service by name
 */
int service_stop(const char *name)
{
    int idx = service_find(name);
    if (idx < 0) {
        printf("service: service '%s' not found\n", name);
        return -1;
    }

    return service_stop_internal(&services[idx]);
}

/*
 * Restart a service
 */
int service_restart(const char *name)
{
    service_update_state(&services[service_find(name)], SERVICE_RESTARTING);
    service_stop(name);
    sys_sleep_ms(500); /* Brief pause */
    return service_start(name);
}

/*
 * Get service status
 */
int service_status(const char *name, service_t *info)
{
    int idx = service_find(name);
    if (idx < 0) return -1;

    memcpy(info, &services[idx], sizeof(service_t));
    return 0;
}

/*
 * List all services
 */
int service_list(service_t *list, int max_count)
{
    int count = service_count < max_count ? service_count : max_count;
    memcpy(list, services, count * sizeof(service_t));
    return count;
}

/*
 * Reload service configuration
 */
int service_reload_config(void)
{
    /* Stop all running services */
    for (int i = 0; i < service_count; i++) {
        if (services[i].state == SERVICE_RUNNING) {
            service_stop_internal(&services[i]);
        }
    }

    /* Reinitialize */
    service_count = 0;
    return service_load_config("/bin/config/services.ini");
}

/*
 * Internal: Find service by name
 */
static int service_find(const char *name)
{
    for (int i = 0; i < service_count; i++) {
        if (strcmp(services[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

/*
 * Internal: Start a service
 */
static int service_start_internal(service_t *svc)
{
    if (svc->state == SERVICE_RUNNING || svc->state == SERVICE_STARTING) {
        return 0; /* Already running */
    }

    service_update_state(svc, SERVICE_STARTING);

    /* Parse command and arguments */
    char cmd[SERVICE_CMD_MAX];
    strcpy(cmd, svc->command);

    char *token = strtok(cmd, " ");
    int argc = 0;
    while (token && argc < 15) {
        svc->argv[argc++] = token;
        token = strtok(NULL, " ");
    }
    svc->argv[argc] = NULL;

    /* Set default environment */
    svc->envp[0] = "PATH=/bin";
    svc->envp[1] = "HOME=/usr/home/guest";
    svc->envp[2] = NULL;

    /* Fork and exec */
    int64_t pid = sys_fork();
    if (pid == 0) {
        /* Child process */
        sys_exec(svc->command, svc->argv, svc->envp);

        /* If we get here, exec failed */
        printf("service: failed to exec '%s'\n", svc->command);
        sys_exit(1);
    } else if (pid > 0) {
        /* Parent process */
        svc->pid = pid;
        svc->start_time = sys_clock_ms();
        service_update_state(svc, SERVICE_RUNNING);
        printf("service: started '%s' (pid=%d)\n", svc->name, (int)pid);
        return 0;
    } else {
        /* Fork failed */
        service_update_state(svc, SERVICE_FAILED);
        printf("service: fork failed for '%s'\n", svc->name);
        return -1;
    }
}

/*
 * Internal: Stop a service
 */
static int service_stop_internal(service_t *svc)
{
    if (svc->state != SERVICE_RUNNING) {
        return 0; /* Not running */
    }

    service_update_state(svc, SERVICE_STOPPING);

    /* Send SIGTERM */
    if (svc->pid > 0) {
        /* For now, just kill the process (assuming SIGTERM = 15) */
        extern int64_t sys_kill(int64_t pid, int sig);
        sys_kill(svc->pid, 15);
    }

    service_update_state(svc, SERVICE_STOPPED);
    svc->pid = 0;
    printf("service: stopped '%s'\n", svc->name);
    return 0;
}

/*
 * Internal: Update service state
 */
static void service_update_state(service_t *svc, service_state_t state)
{
    svc->state = state;
}

/*
 * Check service dependencies
 */
void service_check_dependencies(const char *name)
{
    int idx = service_find(name);
    if (idx < 0) return;

    service_t *svc = &services[idx];

    for (int i = 0; i < svc->depend_count; i++) {
        int dep_idx = service_find(svc->depends_on[i]);
        if (dep_idx >= 0) {
            service_t *dep = &services[dep_idx];
            if (dep->state != SERVICE_RUNNING) {
                printf("service: starting dependency '%s' for '%s'\n",
                       dep->name, svc->name);
                service_start_internal(dep);
            }
        }
    }
}

/*
 * Handle SIGCHLD - child process termination
 */
void service_handle_sigchld(int64_t pid, int status)
{
    for (int i = 0; i < service_count; i++) {
        service_t *svc = &services[i];
        if (svc->pid == pid) {
            printf("service: '%s' (pid=%d) exited with status %d\n",
                   svc->name, (int)pid, status);

            svc->pid = 0;

            if (status != 0 && svc->restart) {
                /* Auto-restart */
                int64_t now = sys_clock_ms();
                if (now - svc->last_restart >= svc->restart_delay) {
                    svc->last_restart = now;
                    svc->restart_count++;
                    printf("service: restarting '%s' (attempt %d)\n",
                           svc->name, svc->restart_count);
                    service_start_internal(svc);
                } else {
                    service_update_state(svc, SERVICE_FAILED);
                }
            } else {
                service_update_state(svc, SERVICE_STOPPED);
            }
            break;
        }
    }
}

/*
 * Service monitoring loop
 */
void service_monitor_loop(void)
{
    while (1) {
        /* Check for dead processes */
        int status;
        int64_t pid = sys_wait(-1, &status);
        if (pid > 0) {
            service_handle_sigchld(pid, status);
        }

        /* Check service health (could be extended) */

        sys_sleep_ms(100);
    }
}