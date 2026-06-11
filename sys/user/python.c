#include "libc.h"
#include "python_runtime.h"

/*
 * Python interpreter entry point
 */

int main(int argc, char **argv)
{
    /* Initialize Python runtime */
    if (py_runtime_init() != 0) {
        printf("python: failed to initialize runtime\n");
        return 1;
    }

    if (argc < 2) {
        /* Interactive mode (very basic) */
        printf("BrightS Python 3.0.0\n");
        printf("Type 'quit()' to exit\n");

        char line[1024];
        while (1) {
            printf(">>> ");
            fflush(stdout);

            /* Read line (simplified) */
            int i = 0;
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF && i < sizeof(line) - 1) {
                line[i++] = ch;
            }
            line[i] = 0;

            if (strcmp(line, "quit()") == 0) break;

            if (py_runtime_execute_string(line, "<stdin>") != 0) {
                printf("Error executing: %s\n", line);
            }
        }

    } else {
        /* Execute file */
        const char *filename = argv[1];

        /* Shift arguments for script */
        char **script_argv = argv + 1;

        if (py_runtime_execute_file(filename, script_argv, NULL) != 0) {
            printf("python: execution failed\n");
            return 1;
        }
    }

    return 0;
}