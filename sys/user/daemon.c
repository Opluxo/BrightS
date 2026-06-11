#include "daemon.h"
#include "libc.h"

#define MAX_DAEMONS 16

static const daemon_info_t *registered_daemons[MAX_DAEMONS];
static int daemon_count = 0;
static volatile int daemon_running = 1;

/*
 * Daemonize the current process
 */
int daemonize(void)
{
    /* Fork to create background process */
    int64_t pid = sys_fork();
    if (pid < 0) {
        return -1; /* Fork failed */
    }

    if (pid > 0) {
        /* Parent exits */
        sys_exit(0);
    }

    /* Child continues as daemon */

    /* Create new session */
    /* Note: sys_setsid may not be implemented yet, skip for now */
    /*
    if (sys_setsid() < 0) {
        return -1;
    }
    */

    /* Change working directory to root */
    sys_chdir("/");

    /* Close standard file descriptors */
    sys_close(0); /* stdin */
    sys_close(1); /* stdout */
    sys_close(2); /* stderr */

    /* Redirect to /dev/null or log files */
    int devnull = sys_open("/dev/null", 0x02); /* O_RDWR */
    if (devnull >= 0) {
        sys_dup2(devnull, 0);
        sys_dup2(devnull, 1);
        sys_dup2(devnull, 2);
        if (devnull > 2) sys_close(devnull);
    }

    return 0;
}

/*
 * Register a daemon
 */
int daemon_register(const daemon_info_t *info)
{
    if (daemon_count >= MAX_DAEMONS) {
        return -1;
    }

    registered_daemons[daemon_count++] = info;
    return 0;
}

/*
 * Start all registered daemons
 */
int daemon_start_all(void)
{
    for (int i = 0; i < daemon_count; i++) {
        const daemon_info_t *info = registered_daemons[i];

        printf("Starting daemon: %s\n", info->name);

        int64_t pid = sys_fork();
        if (pid == 0) {
            /* Child process - become daemon */
            if (daemonize() == 0) {
                /* Write PID file */
                char pidfile[64];
                sprintf(pidfile, "/var/run/%s.pid", info->name);
                daemon_write_pidfile(pidfile);

                /* Initialize daemon */
                if (info->init_func && info->init_func() != 0) {
                    printf("Failed to initialize %s\n", info->name);
                    sys_exit(1);
                }

                /* Main loop */
                while (daemon_running) {
                    if (info->main_loop) {
                        info->main_loop();
                    }
                    sys_sleep_ms(1000);
                }

                /* Cleanup */
                if (info->cleanup_func) {
                    info->cleanup_func();
                }

                daemon_remove_pidfile(pidfile);
            }
            sys_exit(0);

        } else if (pid < 0) {
            printf("Failed to fork %s\n", info->name);
            return -1;
        }
    }

    return 0;
}

/*
 * Stop all daemons
 */
int daemon_stop_all(void)
{
    daemon_running = 0;

    /* Give daemons time to cleanup */
    sys_sleep_ms(2000);

    /* Kill any remaining daemon processes */
    for (int i = 0; i < daemon_count; i++) {
        const daemon_info_t *info = registered_daemons[i];
        char pidfile[64];
        sprintf(pidfile, "/var/run/%s.pid", info->name);

        int fd = sys_open(pidfile, 0); /* O_RDONLY */
        if (fd >= 0) {
            char buf[16];
            int64_t n = sys_read(fd, buf, sizeof(buf) - 1);
            if (n > 0) {
                buf[n] = 0;
                int64_t pid = atoi(buf);
                extern int64_t sys_kill(int64_t pid, int sig);
                sys_kill(pid, 15); /* SIGTERM */
            }
            sys_close(fd);
        }
    }

    return 0;
}

/*
 * Main daemon monitoring loop
 */
void daemon_main_loop(void)
{
    while (daemon_running) {
        /* Check daemon health */
        for (int i = 0; i < daemon_count; i++) {
            const daemon_info_t *info = registered_daemons[i];
            if (info->auto_restart) {
                char pidfile[64];
                sprintf(pidfile, "/var/run/%s.pid", info->name);

                /* Check if PID file exists and process is alive */
                int fd = sys_open(pidfile, 0);
                if (fd < 0) {
                    /* Daemon died, restart it */
                    printf("Restarting daemon: %s\n", info->name);

                    int64_t pid = sys_fork();
                    if (pid == 0) {
                        if (daemonize() == 0) {
                            daemon_write_pidfile(pidfile);
                            if (info->init_func) info->init_func();
                            while (daemon_running) {
                                if (info->main_loop) info->main_loop();
                                sys_sleep_ms(1000);
                            }
                            if (info->cleanup_func) info->cleanup_func();
                            daemon_remove_pidfile(pidfile);
                        }
                        sys_exit(0);
                    }
                } else {
                    sys_close(fd);
                }
            }
        }

        sys_sleep_ms(5000); /* Check every 5 seconds */
    }
}

/*
 * Write PID file
 */
int daemon_write_pidfile(const char *filename)
{
    int fd = sys_open(filename, 0x41); /* O_CREAT | O_WRONLY */
    if (fd < 0) return -1;

    char buf[16];
    int64_t pid = sys_getpid();
    int len = sprintf(buf, "%d\n", (int)pid);

    sys_write(fd, buf, len);
    sys_close(fd);
    return 0;
}

/*
 * Remove PID file
 */
int daemon_remove_pidfile(const char *filename)
{
    sys_unlink(filename);
    return 0;
}

/*
 * Signal handler for daemons
 */
void daemon_signal_handler(int sig)
{
    (void)sig;
    daemon_running = 0;
}