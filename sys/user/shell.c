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
 * Execute a single command with redirection support
 */
static int execute_command(int argc, char **argv)
{
    char *input_file = NULL;
    char *output_file = NULL;
    int append = 0;

    /* Parse redirections */
    int new_argc = 0;
    char *new_argv[MAX_ARGS];

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "<") == 0 && i + 1 < argc) {
            input_file = argv[i + 1];
            i++;  /* Skip next arg */
        } else if (strcmp(argv[i], ">") == 0 && i + 1 < argc) {
            output_file = argv[i + 1];
            append = 0;
            i++;  /* Skip next arg */
        } else if (strcmp(argv[i], ">>") == 0 && i + 1 < argc) {
            output_file = argv[i + 1];
            append = 1;
            i++;  /* Skip next arg */
        } else {
            new_argv[new_argc++] = argv[i];
        }
    }

    /* Handle redirections */
    int saved_stdin = -1;
    int saved_stdout = -1;

    if (input_file) {
        saved_stdin = sys_dup(0);
        int fd = sys_open(input_file, 0);
        if (fd < 0) {
            printf("Cannot open input file: %s\n", input_file);
            return 1;
        }
        sys_dup2(fd, 0);
        sys_close(fd);
    }

    if (output_file) {
        saved_stdout = sys_dup(1);
        int flags = 1;  /* O_CREAT */
        if (append) flags |= 8;  /* O_APPEND */
        int fd = sys_open(output_file, flags);
        if (fd < 0) {
            printf("Cannot open output file: %s\n", output_file);
            if (saved_stdin != -1) {
                sys_dup2(saved_stdin, 0);
                sys_close(saved_stdin);
            }
            return 1;
        }
        sys_dup2(fd, 1);
        sys_close(fd);
    }

    int result;

    /* Check for language-specific execution */
    if (strcmp(new_argv[0], "rust") == 0 && new_argc > 1) {
        /* Execute Rust code */
        result = lang_execute_string(new_argv[1], "rust", "<command>");
    } else if (strcmp(new_argv[0], "python") == 0 && new_argc > 1) {
        /* Execute Python code */
        result = lang_execute_string(new_argv[1], "python", "<command>");
    } else if (strcmp(new_argv[0], "cpp") == 0 && new_argc > 1) {
        /* Execute C++ code */
        result = lang_execute_string(new_argv[1], "cpp", "<command>");
    } else {
        /* Execute regular command */
        result = cmd_execute(new_argc, new_argv);
    }

    /* Restore redirections */
    if (saved_stdin != -1) {
        sys_dup2(saved_stdin, 0);
        sys_close(saved_stdin);
    }
    if (saved_stdout != -1) {
        sys_dup2(saved_stdout, 1);
        sys_close(saved_stdout);
    }

    return result;
}

/*
 * Execute a pipeline of commands
 */
static int execute_pipeline(char *line, int background)
{
    char *commands[10];  /* Max 10 commands in pipeline */
    int cmd_count = 0;

    /* Split by pipes */
    char *token = strtok(line, "|");
    while (token && cmd_count < 10) {
        /* Trim spaces */
        while (*token && (*token == ' ' || *token == '\t')) token++;
        char *end = token + strlen(token) - 1;
        while (end > token && (*end == ' ' || *end == '\t')) {
            *end = 0;
            end--;
        }
        if (*token) {
            commands[cmd_count++] = token;
        }
        token = strtok(NULL, "|");
    }

    if (cmd_count == 1) {
        /* Single command */
        char *argv[MAX_ARGS];
        int argc = parse_args(commands[0], argv);
        if (argc > 0) {
            return execute_command(argc, argv);
        }
        return 0;
    }

    /* Execute pipeline */
    int prev_pipe[2] = {-1, -1};
    int pipes[2];

    for (int i = 0; i < cmd_count; i++) {
        if (i < cmd_count - 1) {
            if (sys_pipe(pipes) != 0) {
                printf("Failed to create pipe\n");
                return 1;
            }
        }

        int64_t pid = sys_fork();
        if (pid == 0) {
            /* Child process */
            if (prev_pipe[0] != -1) {
                /* Redirect stdin from previous pipe */
                sys_close(prev_pipe[1]);
                sys_dup2(prev_pipe[0], 0);  /* stdin */
                sys_close(prev_pipe[0]);
            }
            if (i < cmd_count - 1) {
                /* Redirect stdout to next pipe */
                sys_close(pipes[0]);
                sys_dup2(pipes[1], 1);  /* stdout */
                sys_close(pipes[1]);
            }

            /* Execute command */
            char *argv[MAX_ARGS];
            int argc = parse_args(commands[i], argv);
            if (argc > 0) {
                int result = execute_command(argc, argv);
                sys_exit(result);
            } else {
                sys_exit(0);
            }
        } else if (pid > 0) {
            /* Parent process */
            if (prev_pipe[0] != -1) {
                sys_close(prev_pipe[0]);
                sys_close(prev_pipe[1]);
            }
            if (i < cmd_count - 1) {
                prev_pipe[0] = pipes[0];
                prev_pipe[1] = pipes[1];
            }
        } else {
            printf("Failed to fork\n");
            return 1;
        }
    }

    /* Wait for all children */
    for (int i = 0; i < cmd_count; i++) {
        int status;
        sys_wait(-1, &status);
    }

    return 0;
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

    /* Check for background */
    int background = 0;
    char *ampersand = strstr(line, "&");
    if (ampersand) {
        *ampersand = 0;  /* Remove & */
        background = 1;
    }

    /* Check for pipes */
    if (strchr(line, '|')) {
        return execute_pipeline(line, background);
    } else {
        /* Single command */
        char *argv[MAX_ARGS];
        int argc = parse_args(line, argv);
        if (argc > 0) {
            if (background) {
                int64_t pid = sys_fork();
                if (pid == 0) {
                    /* Child */
                    int result = execute_command(argc, argv);
                    sys_exit(result);
                } else if (pid > 0) {
                    printf("[%lld]\n", pid);  /* Show job ID */
                    return 0;
                } else {
                    printf("Failed to fork\n");
                    return 1;
                }
            } else {
                return execute_command(argc, argv);
            }
        }
    }

    return 0;
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