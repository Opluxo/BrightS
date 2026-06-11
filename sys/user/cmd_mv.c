#include "command.h"
#include "libc.h"

/*
 * mv help
 */
static void cmd_mv_help(void)
{
    printf("Usage: mv SOURCE DEST\n");
    printf("Move/rename files and directories.\n\n");
    printf("Move SOURCE to DEST.\n");
}

/*
 * mv command handler
 */
int cmd_mv_handler(int argc, char **argv)
{
    if (argc < 3) {
        printf("mv: missing operand\n");
        printf("Usage: mv source destination\n");
        return 1;
    }

    const char *src = argv[1];
    const char *dst = argv[2];

    /* For now, implement as copy then remove */
    /* This is not efficient, but works for basic functionality */

    /* First, copy */
    int src_fd = sys_open(src, 0);
    if (src_fd < 0) {
        printf("mv: cannot open '%s'\n", src);
        return 1;
    }

    int dst_fd = sys_create(dst);
    if (dst_fd < 0) {
        sys_close(src_fd);
        printf("mv: cannot create '%s'\n", dst);
        return 1;
    }

    char buf[4096];
    int64_t n;
    while ((n = sys_read(src_fd, buf, sizeof(buf))) > 0) {
        if (sys_write(dst_fd, buf, n) != n) {
            sys_close(src_fd);
            sys_close(dst_fd);
            printf("mv: write error\n");
            return 1;
        }
    }

    sys_close(src_fd);
    sys_close(dst_fd);

    /* Then remove source */
    if (sys_unlink(src) != 0) {
        printf("mv: cannot remove '%s'\n", src);
        return 1;
    }

    return 0;
}