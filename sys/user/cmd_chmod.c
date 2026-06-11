#include "command.h"
#include "libc.h"

/*
 * Parse permission string to mode
 * Supports octal (e.g. 755) and symbolic (not implemented yet)
 */
static uint32_t parse_mode(const char *str)
{
    uint32_t mode = 0;
    if (str[0] >= '0' && str[0] <= '7') {
        /* Octal mode */
        mode = 0;
        while (*str && *str >= '0' && *str <= '7') {
            mode = mode * 8 + (*str - '0');
            ++str;
        }
    } else {
        /* Symbolic mode - not implemented */
        printf("chmod: symbolic modes not supported yet\n");
        return (uint32_t)-1;
    }
    return mode;
}

/*
 * chmod help
 */
static void cmd_chmod_help(void)
{
    printf("Usage: chmod MODE FILE...\n");
    printf("Change file permissions.\n\n");
    printf("MODE can be octal (e.g. 755) or symbolic (not supported yet).\n");
}

/*
 * chmod command handler
 */
int cmd_chmod_handler(int argc, char **argv)
{
    if (argc < 3) {
        printf("chmod: missing operand\n");
        printf("Usage: chmod mode file...\n");
        return 1;
    }

    const char *mode_str = argv[1];
    uint32_t mode = parse_mode(mode_str);
    if (mode == (uint32_t)-1) {
        return 1;
    }

    int errors = 0;
    for (int i = 2; i < argc; ++i) {
        if (sys_chmod(argv[i], mode) != 0) {
            printf("chmod: cannot change permissions of '%s'\n", argv[i]);
            ++errors;
        }
    }

    return errors > 0 ? 1 : 0;
}