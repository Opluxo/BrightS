#include "command.h"
#include "libc.h"

/*
 * Find files recursively
 */
static void find_files(const char *path, const char *name_pattern)
{
    int fd = sys_open(path, 0);
    if (fd < 0) {
        return;
    }

    char buf[4096];
    int64_t n = sys_readdir(fd, buf, sizeof(buf) - 1);
    if (n <= 0) {
        sys_close(fd);
        return;
    }
    buf[n] = 0;
    sys_close(fd);

    /* Parse entries */
    char *entry = buf;
    while (*entry) {
        char *end = entry;
        while (*end && *end != '\n') ++end;
        if (*end == '\n') { *end = 0; ++end; }

        if (entry[0] == 0 || strcmp(entry, ".") == 0 || strcmp(entry, "..") == 0) {
            entry = end;
            continue;
        }

        /* Build full path */
        char fullpath[256];
        int p = 0;
        const char *pp = path;
        while (*pp && p < 250) fullpath[p++] = *pp++;
        if (fullpath[p - 1] != '/') fullpath[p++] = '/';
        int name_len = 0;
        while (entry[name_len] && p + name_len < 250) {
            fullpath[p + name_len] = entry[name_len];
            ++name_len;
        }
        fullpath[p + name_len] = 0;

        /* Check if matches pattern */
        if (strstr(entry, name_pattern)) {
            puts(fullpath);
        }

        /* Recurse if directory */
        uint64_t size;
        uint32_t mode;
        if (sys_stat(fullpath, &size, &mode) == 0 && (mode & 0x4000)) {
            find_files(fullpath, name_pattern);
        }

        entry = end;
    }
}

/*
 * find help
 */
static void cmd_find_help(void)
{
    printf("Usage: find PATH -name PATTERN\n");
    printf("Search for files in PATH matching PATTERN.\n\n");
    printf("Recursively search for files with names containing PATTERN.\n");
}

/*
 * find command handler
 */
int cmd_find_handler(int argc, char **argv)
{
    if (argc < 4 || strcmp(argv[2], "-name") != 0) {
        printf("find: usage: find path -name pattern\n");
        return 1;
    }

    const char *path = argv[1];
    const char *pattern = argv[3];

    find_files(path, pattern);
    return 0;
}