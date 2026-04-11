#include "command.h"
#include "libc.h"
#include "lang_runtime.h"

/*
 * BrightS User Shell
 *
 * A modern command-line interface using the unified command framework
 */

#define MAX_LINE 1024
#define MAX_ARGS 64
#define HISTORY_SIZE 100

/*
 * Read line with history support
 */
static int read_line_with_history(char *line, int max_len, char history[HISTORY_SIZE][MAX_LINE], int *history_count, int *history_index)
{
    int i = 0;
    int ch;

    while (i < max_len - 1) {
        ch = getchar();
        if (ch == '\n') {
            line[i] = 0;
            break;
        } else if (ch == '\033') {  /* Escape sequence */
            ch = getchar();
            if (ch == '[') {
                ch = getchar();
                if (ch == 'A') {  /* Up arrow */
                    if (*history_index > 0) {
                        (*history_index)--;
                        strcpy(line, history[*history_index]);
                        printf("\r$ %s", line);
                        fflush(stdout);
                        i = strlen(line);
                    }
                } else if (ch == 'B') {  /* Down arrow */
                    if (*history_index < *history_count - 1) {
                        (*history_index)++;
                        strcpy(line, history[*history_index]);
                        printf("\r$ %s", line);
                        fflush(stdout);
                        i = strlen(line);
                    } else if (*history_index == *history_count - 1) {
                        (*history_index) = *history_count;
                        line[0] = 0;
                        printf("\r$ ");
                        fflush(stdout);
                        i = 0;
                    }
                }
            }
        } else if (ch == '\t') {  /* Tab for completion */
            /* Find partial command */
            line[i] = 0;
            char *partial = line;
            /* Skip to last word */
            char *last_space = strrchr(line, ' ');
            if (last_space) {
                partial = last_space + 1;
            }

            /* Find matching commands */
            command_t commands[MAX_COMMANDS];
            int count = cmd_list(commands, MAX_COMMANDS, CMD_CAT_MAX);  /* All categories */

            char *match = NULL;
            int match_count = 0;
            for (int j = 0; j < count; j++) {
                if (strncmp(commands[j].name, partial, strlen(partial)) == 0) {
                    if (match_count == 0) {
                        match = commands[j].name;
                    }
                    match_count++;
                }
            }

            if (match_count == 1) {
                /* Complete */
                strcpy(partial, match);
                printf("\r$ %s", line);
                fflush(stdout);
                i = strlen(line);
            } else if (match_count > 1) {
                /* Show possibilities */
                printf("\n");
                for (int j = 0; j < count; j++) {
                    if (strncmp(commands[j].name, partial, strlen(partial)) == 0) {
                        printf("%s ", commands[j].name);
                    }
                }
                printf("\n$ %s", line);
                fflush(stdout);
            }
        } else if (ch == 127 || ch == '\b') {  /* Backspace */
            if (i > 0) {
                i--;
                printf("\b \b");
                fflush(stdout);
            }
        } else {
            line[i++] = ch;
            putchar(ch);
            fflush(stdout);
        }
    }

    if (i > 0) {
        /* Add to history */
        if (*history_count < HISTORY_SIZE) {
            strcpy(history[*history_count], line);
            (*history_count)++;
        } else {
            /* Shift */
            for (int j = 1; j < HISTORY_SIZE; j++) {
                strcpy(history[j-1], history[j]);
            }
            strcpy(history[HISTORY_SIZE-1], line);
        }
        *history_index = *history_count;
    }

    return i;
}

/*
 * Parse command line into arguments
 */
static int parse_args(char *line, char **argv)
{
    int argc = 0;
    char *p = line;

    while (*p && argc < MAX_ARGS - 1) {
        /* Skip whitespace */
        while (*p && (*p == ' ' || *p == '\t')) p++;

        if (!*p) break;

        /* Handle quotes */
        char quote = 0;
        if (*p == '"' || *p == '\'') {
            quote = *p++;
        }

        argv[argc++] = p;

        /* Find end of argument */
        while (*p) {
            if (quote) {
                if (*p == quote) {
                    *p++ = 0;
                    break;
                }
            } else {
                if (*p == ' ' || *p == '\t') {
                    *p++ = 0;
                    break;
                }
            }
            p++;
        }
    }

    argv[argc] = NULL;
    return argc;
}

/*
 * Execute a command line
 */
static int execute_line(char *line)
{
    /* Remove trailing newline */
    char *p = line;
    while (*p) {
        if (*p == '\n' || *p == '\r') {
            *p = 0;
            break;
        }
        p++;
    }

    /* Skip empty lines */
    p = line;
    while (*p && (*p == ' ' || *p == '\t')) p++;
    if (!*p) return 0;

    /* Parse arguments */
    char *argv[MAX_ARGS];
    int argc = parse_args(line, argv);

    if (argc == 0) return 0;

    /* Check for language-specific execution */
    if (strcmp(argv[0], "rust") == 0 && argc > 1) {
        /* Execute Rust code */
        return lang_execute_string(argv[1], "rust", "<command>");
    } else if (strcmp(argv[0], "python") == 0 && argc > 1) {
        /* Execute Python code */
        return lang_execute_string(argv[1], "python", "<command>");
    } else if (strcmp(argv[0], "cpp") == 0 && argc > 1) {
        /* Execute C++ code */
        return lang_execute_string(argv[1], "cpp", "<command>");
    } else {
        /* Execute regular command */
        return cmd_execute(argc, argv);
    }
}

/*
 * Main shell loop
 */
int main(int argc, char **argv)
{
    (void)argc; (void)argv;

    printf("BrightS Shell v2.0\n");
    printf("Type 'help' for available commands\n");
    printf("Use 'rust \"code\"', 'python \"code\"', 'cpp \"code\"' for language execution\n\n");

    /* Initialize command system */
    cmd_init();
    cmd_register_all();

    /* Initialize language runtimes */
    lang_init();

    /* Register language runtimes */
    extern runtime_t *rust_create_runtime(void);
    extern runtime_t *py_create_runtime(void);
    extern runtime_t *cpp_create_runtime(void);

    lang_register_runtime(rust_create_runtime());
    lang_register_runtime(py_create_runtime());
    lang_register_runtime(cpp_create_runtime());

    char line[MAX_LINE];
    char history[HISTORY_SIZE][MAX_LINE];
    int history_count = 0;
    int history_index = -1;

    while (1) {
        printf("$ ");
        fflush(stdout);

        /* Read line with history */
        int len = read_line_with_history(line, sizeof(line), history, &history_count, &history_index);

        if (len < 0) break;  /* EOF */

        /* Execute command */
        if (execute_line(line) != 0) {
            /* Command failed */
        }
    }

    printf("\nGoodbye!\n");
    return 0;
}