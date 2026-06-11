#include "command.h"
#include "libc.h"

/*
 * cp help
 */
static void cmd_cp_help(void)
{
    printf("Usage: cp SOURCE DEST\n");
    printf("Copy files and directories.\n\n");
    printf("Copy SOURCE to DEST.\n");
}

/*
 * Copy recursively
 */
static int copy_recursive(const char *src, const char *dst)
{
    uint64_t size;
    uint32_t mode;
    if (sys_stat(src, &size, &mode) != 0) {
        return -1;
    }

    if ((mode & 0x4000)) {  /* Directory */
        /* Create destination directory */
        if (sys_mkdir(dst) != 0) {
            return -1;
        }

        int src_fd = sys_open(src, 0);
        if (src_fd < 0) return -1;

        char buf[4096];
        int64_t n = sys_readdir(src_fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = 0;
            char *entry = buf;
            while (*entry) {
                char *end = entry;
                while (*end && *end != '\n') ++end;
                if (*end == '\n') { *end = 0; ++end; }

                if (strcmp(entry, ".") == 0 || strcmp(entry, "..") == 0) {
                    entry = end;
                    continue;
                }

                /* Build paths */
                char src_path[256], dst_path[256];
                int src_len = strlen(src);
                int dst_len = strlen(dst);
                if (src_len + strlen(entry) + 2 > sizeof(src_path) ||
                    dst_len + strlen(entry) + 2 > sizeof(dst_path)) {
                    entry = end;
                    continue;
                }
                strcpy(src_path, src);
                if (src_path[src_len - 1] != '/') src_path[src_len++] = '/';
                strcpy(src_path + src_len, entry);

                strcpy(dst_path, dst);
                if (dst_path[dst_len - 1] != '/') dst_path[dst_len++] = '/';
                strcpy(dst_path + dst_len, entry);

                if (copy_recursive(src_path, dst_path) != 0) {
                    sys_close(src_fd);
                    return -1;
                }
                entry = end;
            }
        }
        sys_close(src_fd);
    } else {  /* File */
        int src_fd = sys_open(src, 0);
        if (src_fd < 0) return -1;

        int dst_fd = sys_create(dst);
        if (dst_fd < 0) {
            sys_close(src_fd);
            return -1;
        }

        char buf[4096];
        int64_t n;
        while ((n = sys_read(src_fd, buf, sizeof(buf))) > 0) {
            if (sys_write(dst_fd, buf, n) != n) {
                sys_close(src_fd);
                sys_close(dst_fd);
                return -1;
            }
        }

        sys_close(src_fd);
        sys_close(dst_fd);

        if (n < 0) return -1;
    }

    return 0;
}

/*
 * cp command handler
 */
int cmd_cp_handler(int argc, char **argv)
{
    int recursive = 0;

    /* Parse arguments */
    int i = 1;
    if (argc > 1 && argv[1][0] == '-') {
        if (strcmp(argv[1], "-r") == 0) {
            recursive = 1;
            i = 2;
        }
    }

    if (argc - i < 2) {
        printf("cp: missing operand\n");
        printf("Usage: cp [-r] source destination\n");
        return 1;
    }

    const char *src = argv[i];
    const char *dst = argv[i + 1];

    int result = recursive ? copy_recursive(src, dst) : copy_recursive(src, dst);

    if (result != 0) {
        printf("cp: cannot copy '%s' to '%s'\n", src, dst);
        return 1;
    }

    return 0;
}