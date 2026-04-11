#include "command.h"
#include "libc.h"

/*
 * echo command handler
 */
int cmd_echo_handler(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        if (i > 1) printf(" ");
        printf("%s", argv[i]);
    }
    printf("\n");
    return 0;
}

/*
 * pwd command handler
 */
int cmd_pwd_handler(int argc, char **argv)
{
    (void)argc; (void)argv;
    char buf[256];
    if (sys_getcwd(buf, sizeof(buf)) == 0) {
        printf("%s\n", buf);
        return 0;
    }
    printf("pwd: cannot get current directory\n");
    return 1;
}

/*
 * cd command handler
 */
int cmd_cd_handler(int argc, char **argv)
{
    const char *dir = (argc > 1) ? argv[1] : "/usr/home/guest";
    if (sys_chdir(dir) == 0) {
        return 0;
    }
    printf("cd: cannot change to %s\n", dir);
    return 1;
}

/*
 * cat command handler
 */
int cmd_cat_handler(int argc, char **argv)
{
    if (argc < 2) {
        printf("cat: missing file operand\n");
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        int fd = sys_open(argv[i], 0); /* O_RDONLY */
        if (fd < 0) {
            printf("cat: cannot open %s\n", argv[i]);
            continue;
        }

        char buf[4096];
        int64_t n;
        while ((n = sys_read(fd, buf, sizeof(buf))) > 0) {
            sys_write(1, buf, n);
        }
        sys_close(fd);
    }
    return 0;
}

/*
 * mkdir command handler
 */
int cmd_mkdir_handler(int argc, char **argv)
{
    if (argc < 2) {
        printf("mkdir: missing operand\n");
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        if (sys_mkdir(argv[i]) != 0) {
            printf("mkdir: cannot create directory %s\n", argv[i]);
            return 1;
        }
    }
    return 0;
}