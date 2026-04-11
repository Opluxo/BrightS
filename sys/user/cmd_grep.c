#include "command.h"
#include "libc.h"

/*
 * grep help
 */
static void cmd_grep_help(void)
{
    printf("Usage: grep PATTERN FILE...\n");
    printf("Search for PATTERN in each FILE.\n\n");
    printf("Print lines matching PATTERN.\n");
}

/*
 * grep command handler
 */
int cmd_grep_handler(int argc, char **argv)
{
    if (argc < 3) {
        printf("grep: missing pattern or file\n");
        printf("Usage: grep pattern file...\n");
        return 1;
    }

    const char *pattern = argv[1];
    int pattern_len = strlen(pattern);

    int found = 0;
    for (int i = 2; i < argc; ++i) {
        const char *filename = argv[i];

        int fd = sys_open(filename, 0);
        if (fd < 0) {
            printf("grep: cannot open '%s'\n", filename);
            continue;
        }

        char buf[4096];
        int64_t n;
        int line_num = 1;
        char line[1024];
        int line_pos = 0;

        while ((n = sys_read(fd, buf, sizeof(buf))) > 0) {
            for (int j = 0; j < n; ++j) {
                if (buf[j] == '\n' || line_pos >= sizeof(line) - 1) {
                    line[line_pos] = 0;
                    /* Search for pattern in line */
                    if (strstr(line, pattern)) {
                        printf("%s:%d:%s\n", filename, line_num, line);
                        found = 1;
                    }
                    line_pos = 0;
                    ++line_num;
                } else {
                    line[line_pos++] = buf[j];
                }
            }
        }

        /* Handle last line if no newline */
        if (line_pos > 0) {
            line[line_pos] = 0;
            if (strstr(line, pattern)) {
                printf("%s:%d:%s\n", filename, line_num, line);
                found = 1;
            }
        }

        sys_close(fd);
    }

    return found ? 0 : 1;
}