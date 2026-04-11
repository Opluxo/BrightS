#include "command.h"
#include "libc.h"

static command_t commands[MAX_COMMANDS];
static int command_count = 0;

/* Category names */
static const char *category_names[CMD_CAT_MAX] = {
    "File Operations",
    "Network Tools",
    "System Management",
    "Development Tools",
    "Utility Commands",
    "Administrative"
};

/*
 * Initialize command system
 */
int cmd_init(void)
{
    memset(commands, 0, sizeof(commands));
    command_count = 0;

    /* Register built-in commands */
    cmd_register(&(command_t){
        .name = "help",
        .description = "Show help information",
        .category = CMD_CAT_UTILITY,
        .flags = CMD_FLAG_PIPE_OUT,
        .handler = cmd_help_handler,
        .help_func = NULL
    });

    cmd_register(&(command_t){
        .name = "exit",
        .description = "Exit the shell",
        .category = CMD_CAT_SYSTEM,
        .flags = 0,
        .handler = cmd_exit_handler,
        .help_func = NULL
    });

    cmd_register(&(command_t){
        .name = "clear",
        .description = "Clear the screen",
        .category = CMD_CAT_UTILITY,
        .flags = 0,
        .handler = cmd_clear_handler,
        .help_func = NULL
    });

    printf("cmd: command system initialized\n");
    return 0;
}

/*
 * Register a command
 */
int cmd_register(const command_t *cmd)
{
    if (command_count >= MAX_COMMANDS) {
        printf("cmd: too many commands registered\n");
        return -1;
    }

    memcpy(&commands[command_count], cmd, sizeof(command_t));
    command_count++;

    return 0;
}

/*
 * Execute a command
 */
int cmd_execute(int argc, char **argv)
{
    if (argc < 1) return -1;

    const char *cmd_name = argv[0];

    /* Find command */
    command_t cmd;
    if (cmd_find(cmd_name, &cmd) != 0) {
        printf("cmd: command '%s' not found\n", cmd_name);
        printf("Type 'help' for available commands\n");
        return -1;
    }

    /* Execute command */
    return cmd.handler(argc, argv);
}

/*
 * Find a command by name
 */
int cmd_find(const char *name, command_t *cmd)
{
    for (int i = 0; i < command_count; i++) {
        if (strcmp(commands[i].name, name) == 0) {
            memcpy(cmd, &commands[i], sizeof(command_t));
            return 0;
        }
    }
    return -1;
}

/*
 * List commands by category
 */
int cmd_list(command_t *list, int max_count, command_category_t category)
{
    int count = 0;
    for (int i = 0; i < command_count; i++) {
        if (category == CMD_CAT_MAX || commands[i].category == category) {
            if (count < max_count) {
                memcpy(&list[count], &commands[i], sizeof(command_t));
                count++;
            }
        }
    }
    return count;
}

/*
 * Show help for a command
 */
void cmd_help(const char *command_name)
{
    if (!command_name) {
        cmd_list_all();
        return;
    }

    command_t cmd;
    if (cmd_find(command_name, &cmd) == 0) {
        printf("Command: %s\n", cmd.name);
        printf("Category: %s\n", cmd_category_name(cmd.category));
        printf("Description: %s\n", cmd.description);

        if (cmd.flags & CMD_FLAG_NEEDS_ROOT) {
            printf("Requires: Root privileges\n");
        }
        if (cmd.flags & CMD_FLAG_BACKGROUND_OK) {
            printf("Features: Can run in background\n");
        }
        if (cmd.flags & CMD_FLAG_PIPE_IN) {
            printf("Features: Accepts pipe input\n");
        }
        if (cmd.flags & CMD_FLAG_PIPE_OUT) {
            printf("Features: Can pipe output\n");
        }

        if (cmd.help_func) {
            printf("\n");
            cmd.help_func();
        }
    } else {
        printf("Command '%s' not found\n", command_name);
    }
}

/*
 * List all commands
 */
void cmd_list_all(void)
{
    printf("BrightS System Commands\n");
    printf("======================\n\n");

    for (int cat = 0; cat < CMD_CAT_MAX; cat++) {
        command_t cat_cmds[MAX_COMMANDS];
        int count = cmd_list(cat_cmds, MAX_COMMANDS, cat);

        if (count > 0) {
            printf("%s:\n", category_names[cat]);
            for (int i = 0; i < count; i++) {
                printf("  %-12s %s\n", cat_cmds[i].name, cat_cmds[i].description);
            }
            printf("\n");
        }
    }

    printf("Type 'help <command>' for detailed help\n");
}

/*
 * Get category name
 */
const char *cmd_category_name(command_category_t cat)
{
    if (cat >= 0 && cat < CMD_CAT_MAX) {
        return category_names[cat];
    }
    return "Unknown";
}

/*
 * Built-in command handlers
 */
int cmd_help_handler(int argc, char **argv)
{
    if (argc > 1) {
        cmd_help(argv[1]);
    } else {
        cmd_list_all();
    }
    return 0;
}

int cmd_exit_handler(int argc, char **argv)
{
    (void)argc; (void)argv;
    printf("Goodbye!\n");
    sys_exit(0);
    return 0;
}

int cmd_clear_handler(int argc, char **argv)
{
    (void)argc; (void)argv;
    /* ANSI escape sequence to clear screen */
    printf("\033[2J\033[H");
    return 0;
}

int cmd_history_handler(int argc, char **argv)
{
    (void)argc; (void)argv;
    /* TODO: Implement command history */
    printf("Command history not implemented yet\n");
    return 0;
}