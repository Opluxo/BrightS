#include "command.h"
#include "libc.h"

/*
 * rm help
 */
static void cmd_rm_help(void)
{
    printf("Usage: rm [OPTIONS] FILE...\n");
    printf("Remove files and directories.\n\n");
    printf("Options:\n");
    printf("  -f    force removal\n");
    printf("  -r    remove directories recursively\n");
}

/*
 * Remove directory recursively
 */
static int remove_recursive(const char *path)
{
    uint64_t size;
    uint32_t mode;
    if (sys_stat(path, &size, &mode) != 0) {
        return -1;  /* Cannot stat */
    }

    if ((mode & 0x4000)) {  /* Directory */
        int fd = sys_open(path, 0);
        if (fd < 0) return -1;

        char buf[4096];
        int64_t n = sys_readdir(fd, buf, sizeof(buf) - 1);
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

                /* Build full path */
                char fullpath[256];
                int len = strlen(path);
                if (len + strlen(entry) + 2 > sizeof(fullpath)) {
                    entry = end;
                    continue;
                }
                strcpy(fullpath, path);
                if (fullpath[len - 1] != '/') fullpath[len++] = '/';
                strcpy(fullpath + len, entry);

                if (remove_recursive(fullpath) != 0) {
                    sys_close(fd);
                    return -1;
                }
                entry = end;
            }
        }
        sys_close(fd);
    }

    return sys_unlink(path);
}

/*
 * rm command handler
 */
int cmd_rm_handler(int argc, char **argv)
{
    int force = 0;
    int recursive = 0;

    /* Parse arguments */
    int i;
    for (i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            for (int j = 1; argv[i][j]; ++j) {
                if (argv[i][j] == 'f') force = 1;
                else if (argv[i][j] == 'r') recursive = 1;
            }
        } else {
            break;
        }
    }

    if (i >= argc) {
        printf("rm: missing operand\n");
        return 1;
    }

    int errors = 0;
    for (; i < argc; ++i) {
        const char *path = argv[i];

        int result = recursive ? remove_recursive(path) : sys_unlink(path);

        if (result != 0 && !force) {
            uint64_t size;
            uint32_t mode;
            if (sys_stat(path, &size, &mode) == 0 && (mode & 0x4000) && !recursive) {
                printf("rm: cannot remove '%s': Is a directory\n", path);
            } else {
                printf("rm: cannot remove '%s'\n", path);
            }
            ++errors;
        }
    }

    return errors > 0 ? 1 : 0;
}