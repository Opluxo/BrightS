#include "command.h"
#include "libc.h"

/* Forward declarations for command handlers */
int cmd_ls_handler(int argc, char **argv);
int cmd_cat_handler(int argc, char **argv);
int cmd_echo_handler(int argc, char **argv);
int cmd_pwd_handler(int argc, char **argv);
int cmd_cd_handler(int argc, char **argv);
int cmd_mkdir_handler(int argc, char **argv);
int cmd_rm_handler(int argc, char **argv);
int cmd_cp_handler(int argc, char **argv);
int cmd_mv_handler(int argc, char **argv);
int cmd_chmod_handler(int argc, char **argv);
int cmd_grep_handler(int argc, char **argv);
int cmd_find_handler(int argc, char **argv);
int cmd_ping_handler(int argc, char **argv);
int cmd_ps_handler(int argc, char **argv);

/*
 * Register all system commands
 */
int cmd_register_all(void)
{
    /* File operations */
    cmd_register(&(command_t){
        .name = "ls",
        .description = "List directory contents",
        .category = CMD_CAT_FILE,
        .flags = CMD_FLAG_PIPE_OUT,
        .handler = cmd_ls_handler,
        .help_func = NULL
    });

    cmd_register(&(command_t){
        .name = "cat",
        .description = "Concatenate and display files",
        .category = CMD_CAT_FILE,
        .flags = CMD_FLAG_PIPE_IN | CMD_FLAG_PIPE_OUT,
        .handler = cmd_cat_handler,
        .help_func = NULL
    });

    cmd_register(&(command_t){
        .name = "cp",
        .description = "Copy files and directories",
        .category = CMD_CAT_FILE,
        .flags = 0,
        .handler = cmd_cp_handler,
        .help_func = NULL
    });

    cmd_register(&(command_t){
        .name = "mv",
        .description = "Move/rename files and directories",
        .category = CMD_CAT_FILE,
        .flags = 0,
        .handler = cmd_mv_handler,
        .help_func = NULL
    });

    cmd_register(&(command_t){
        .name = "rm",
        .description = "Remove files and directories",
        .category = CMD_CAT_FILE,
        .flags = 0,
        .handler = cmd_rm_handler,
        .help_func = NULL
    });

    cmd_register(&(command_t){
        .name = "mkdir",
        .description = "Create directories",
        .category = CMD_CAT_FILE,
        .flags = 0,
        .handler = cmd_mkdir_handler,
        .help_func = NULL
    });

    /* Utility commands */
    cmd_register(&(command_t){
        .name = "echo",
        .description = "Display text",
        .category = CMD_CAT_UTILITY,
        .flags = CMD_FLAG_PIPE_OUT,
        .handler = cmd_echo_handler,
        .help_func = NULL
    });

    cmd_register(&(command_t){
        .name = "pwd",
        .description = "Print working directory",
        .category = CMD_CAT_UTILITY,
        .flags = CMD_FLAG_PIPE_OUT,
        .handler = cmd_pwd_handler,
        .help_func = NULL
    });

    cmd_register(&(command_t){
        .name = "cd",
        .description = "Change directory",
        .category = CMD_CAT_UTILITY,
        .flags = 0,
        .handler = cmd_cd_handler,
        .help_func = NULL
    });

    cmd_register(&(command_t){
        .name = "cat",
        .description = "Concatenate and display files",
        .category = CMD_CAT_FILE,
        .flags = CMD_FLAG_PIPE_IN | CMD_FLAG_PIPE_OUT,
        .handler = cmd_cat_handler,
        .help_func = NULL
    });

    cmd_register(&(command_t){
        .name = "mkdir",
        .description = "Create directories",
        .category = CMD_CAT_FILE,
        .flags = 0,
        .handler = cmd_mkdir_handler,
        .help_func = NULL
    });

    cmd_register(&(command_t){
        .name = "grep",
        .description = "Search for patterns in files",
        .category = CMD_CAT_UTILITY,
        .flags = CMD_FLAG_PIPE_IN | CMD_FLAG_PIPE_OUT,
        .handler = cmd_grep_handler,
        .help_func = NULL
    });

    cmd_register(&(command_t){
        .name = "find",
        .description = "Search for files",
        .category = CMD_CAT_UTILITY,
        .flags = CMD_FLAG_PIPE_OUT,
        .handler = cmd_find_handler,
        .help_func = NULL
    });

    /* Network commands */
    cmd_register(&(command_t){
        .name = "ping",
        .description = "Send ICMP echo requests",
        .category = CMD_CAT_NETWORK,
        .flags = 0,
        .handler = cmd_ping_handler,
        .help_func = NULL
    });

    /* System commands */
    cmd_register(&(command_t){
        .name = "ps",
        .description = "Display process information",
        .category = CMD_CAT_SYSTEM,
        .flags = CMD_FLAG_PIPE_OUT,
        .handler = cmd_ps_handler,
        .help_func = NULL
    });

    /* Administrative commands */
    cmd_register(&(command_t){
        .name = "chmod",
        .description = "Change file permissions",
        .category = CMD_CAT_ADMIN,
        .flags = CMD_FLAG_NEEDS_ROOT,
        .handler = cmd_chmod_handler,
        .help_func = NULL
    });

    return 0;
}