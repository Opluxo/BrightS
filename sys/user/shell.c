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

    while (1) {
        printf("$ ");
        fflush(stdout);

        /* Read line */
        int i = 0;
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF && i < sizeof(line) - 1) {
            line[i++] = ch;
        }
        line[i] = 0;

        if (ch == EOF) break;

        /* Execute command */
        if (execute_line(line) != 0) {
            /* Command failed */
        }
    }

    printf("\nGoodbye!\n");
    return 0;
}