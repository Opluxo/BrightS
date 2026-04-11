#ifndef BRIGHTS_COMMAND_H
#define BRIGHTS_COMMAND_H

#include <stdint.h>

#define MAX_COMMAND_NAME 32
#define MAX_COMMAND_DESC 128
#define MAX_COMMANDS 128

/* Command categories */
typedef enum {
    CMD_CAT_FILE = 0,      /* File operations: ls, cp, mv, rm, mkdir, etc. */
    CMD_CAT_NETWORK,       /* Network tools: ping, ifconfig, netstat */
    CMD_CAT_SYSTEM,        /* System management: ps, kill, service, etc. */
    CMD_CAT_DEVELOPMENT,   /* Development tools: gcc, python, rustc, etc. */
    CMD_CAT_UTILITY,       /* Utility commands: echo, cat, grep, etc. */
    CMD_CAT_ADMIN,         /* Administrative: chmod, chown, mount, etc. */
    CMD_CAT_MAX
} command_category_t;

/* Command flags */
#define CMD_FLAG_NEEDS_ROOT    (1 << 0)  /* Requires root privileges */
#define CMD_FLAG_BACKGROUND_OK (1 << 1)  /* Can run in background */
#define CMD_FLAG_PIPE_IN       (1 << 2)  /* Accepts pipe input */
#define CMD_FLAG_PIPE_OUT      (1 << 3)  /* Can pipe output */

/* Command structure */
typedef struct {
    char name[MAX_COMMAND_NAME];
    char description[MAX_COMMAND_DESC];
    command_category_t category;
    uint32_t flags;
    int (*handler)(int argc, char **argv);
    void (*help_func)(void);
} command_t;

/* Command framework API */
int cmd_init(void);
int cmd_register(const command_t *cmd);
int cmd_execute(int argc, char **argv);
int cmd_list(command_t *list, int max_count, command_category_t category);
int cmd_find(const char *name, command_t *cmd);
void cmd_help(const char *command_name);
void cmd_list_all(void);
const char *cmd_category_name(command_category_t cat);

/* Built-in commands */
int cmd_help_handler(int argc, char **argv);
int cmd_exit_handler(int argc, char **argv);
int cmd_clear_handler(int argc, char **argv);
int cmd_history_handler(int argc, char **argv);

#endif