#include "command.h"
#include "libc.h"

/*
 * Simple glob pattern matching
 */
static int match_pattern(const char *pattern, const char *str)
{
    if (!pattern || !str) return 0;

    while (*pattern && *str) {
        if (*pattern == '*') {
            /* Wildcard: match zero or more characters */
            pattern++;
            if (!*pattern) return 1;  /* * at end matches */
            while (*str) {
                if (match_pattern(pattern, str)) return 1;
                str++;
            }
            return 0;
        } else if (*pattern == '?' || *pattern == *str) {
            pattern++;
            str++;
        } else {
            return 0;
        }
    }

    return *pattern == 0 && *str == 0;
}

/*
 * List directory contents with pattern matching
 */
static void list_dir(const char *path, const char *pattern, int long_format)
{
    int fd = sys_open(path, 0);
    if (fd < 0) {
        printf("ls: cannot open %s\n", path);
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

    /* Parse newline-separated entries */
    char *entry = buf;
    while (*entry) {
        char *end = entry;
        while (*end && *end != '\n') ++end;
        if (*end == '\n') { *end = 0; ++end; }

        if (entry[0] == 0) { entry = end; continue; }

        /* Check pattern match */
        if (pattern && !match_pattern(pattern, entry)) { entry = end; continue; }

        if (long_format) {
            /* Build full path for stat */
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

            uint64_t size = 0;
            uint32_t mode = 0;
            if (sys_stat(fullpath, &size, &mode) == 0) {
                /* Print mode bits */
                if (mode & 0x4000) putchar('d');
                else if (mode & 0xA000) putchar('l');
                else putchar('-');
                putchar(mode & 0x100 ? 'r' : '-');
                putchar(mode & 0x080 ? 'w' : '-');
                putchar(mode & 0x040 ? 'x' : '-');
                putchar(mode & 0x020 ? 'r' : '-');
                putchar(mode & 0x010 ? 'w' : '-');
                putchar(mode & 0x008 ? 'x' : '-');
                putchar(mode & 0x004 ? 'r' : '-');
                putchar(mode & 0x002 ? 'w' : '-');
                putchar(mode & 0x001 ? 'x' : '-');
                printf(" %8u ", (uint32_t)size);
            }
        }

        puts(entry);
        entry = end;
    }
}

/*
 * ls command handler
 */
int cmd_ls_handler(int argc, char **argv)
{
    int long_format = 0;
    const char *path = ".";
    const char *pattern = NULL;

    /* Parse arguments */
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            for (int j = 1; argv[i][j]; ++j) {
                if (argv[i][j] == 'l') long_format = 1;
            }
        } else {
            if (!pattern) {
                /* Check if contains wildcard */
                if (strchr(argv[i], '*') || strchr(argv[i], '?')) {
                    pattern = argv[i];
                } else {
                    path = argv[i];
                }
            } else {
                /* Multiple patterns not supported yet */
                printf("ls: too many arguments\n");
                return 1;
            }
        }
    }

    /* List directory */
    list_dir(path, pattern, long_format);
    return 0;
}