#include "daemon.h"
#include "libc.h"

/*
 * System Logging Daemon (syslogd)
 */

#define LOG_BUFFER_SIZE 4096
#define LOG_MAX_LINE 256

static char log_buffer[LOG_BUFFER_SIZE];
static int log_pos = 0;

/*
 * Initialize syslog daemon
 */
static int syslogd_init(void)
{
    /* Create log directory */
    sys_mkdir("/var/log");

    /* Open log file */
    int fd = sys_open("/var/log/system.log", 0x41); /* O_CREAT | O_WRONLY */
    if (fd >= 0) {
        sys_close(fd);
    }

    printf("syslogd: initialized\n");
    return 0;
}

/*
 * Write log message to file
 */
static void syslog_write(const char *msg)
{
    int fd = sys_open("/var/log/system.log", 0x402); /* O_APPEND | O_WRONLY */
    if (fd >= 0) {
        char timestamp[32];
        int64_t now = sys_clock_ms();
        sprintf(timestamp, "[%lld] ", now);

        sys_write(fd, timestamp, strlen(timestamp));
        sys_write(fd, msg, strlen(msg));
        sys_write(fd, "\n", 1);
        sys_close(fd);
    }
}

/*
 * Process log buffer
 */
static void syslog_process_buffer(void)
{
    if (log_pos == 0) return;

    /* Null terminate */
    log_buffer[log_pos] = 0;

    /* Write to log file */
    syslog_write(log_buffer);

    /* Reset buffer */
    log_pos = 0;
}

/*
 * Main syslog daemon loop
 */
static void syslogd_main(void)
{
    /* Create log socket for receiving messages */
    int sockfd = sys_socket(2, 2, 0); /* AF_INET, SOCK_DGRAM, IPPROTO_UDP */
    if (sockfd < 0) {
        printf("syslogd: failed to create socket\n");
        return;
    }

    /* Bind to syslog port (514) */
    if (sys_bind(sockfd, 0, 514) < 0) {
        printf("syslogd: failed to bind to port 514\n");
        sys_close_socket(sockfd);
        return;
    }

    printf("syslogd: listening on port 514\n");

    char buf[LOG_MAX_LINE];
    while (1) {
        int64_t n = sys_recv(sockfd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = 0;

            /* Simple log level parsing */
            char *level = "INFO";
            if (strstr(buf, "ERROR")) level = "ERROR";
            else if (strstr(buf, "WARN")) level = "WARN";
            else if (strstr(buf, "DEBUG")) level = "DEBUG";

            /* Format log message */
            char log_msg[LOG_MAX_LINE + 32];
            sprintf(log_msg, "%s: %s", level, buf);

            syslog_write(log_msg);
        }

        sys_sleep_ms(100);
    }

    sys_close_socket(sockfd);
}

/*
 * Cleanup syslog daemon
 */
static void syslogd_cleanup(void)
{
    syslog_process_buffer();
    printf("syslogd: shutdown\n");
}

/*
 * Syslog daemon info
 */
const daemon_info_t syslogd_info = {
    .name = "syslogd",
    .init_func = syslogd_init,
    .main_loop = syslogd_main,
    .cleanup_func = syslogd_cleanup,
    .auto_restart = 1,
    .restart_delay_ms = 5000
};