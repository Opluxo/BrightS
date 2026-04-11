#include "command.h"
#include "libc.h"

/*
 * ps help
 */
static void cmd_ps_help(void)
{
    printf("Usage: ps\n");
    printf("Display process information.\n\n");
    printf("Show current running processes.\n");
}

/*
 * ps command handler
 */
int cmd_ps_handler(int argc, char **argv)
{
    printf("PID   PPID  STATE\n");

    /* For now, only show current process */
    int64_t pid = sys_getpid();
    int64_t ppid = sys_getppid();

    printf("%lld  %lld  running\n", pid, ppid);

    /* Note: Full ps would require process enumeration syscall */
    /* This is a simplified implementation */

    return 0;
}