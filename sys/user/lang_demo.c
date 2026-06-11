#include "libc.h"
#include "lang_runtime.h"

/*
 * Multi-language demonstration program
 */

int main(int argc, char **argv)
{
    printf("BrightS Multi-Language Runtime Demo\n");
    printf("===================================\n\n");

    /* Initialize language system */
    if (lang_init() != 0) {
        printf("Failed to initialize language system\n");
        return 1;
    }

    /* Register runtimes */
    extern runtime_t *rust_create_runtime(void);
    extern runtime_t *py_create_runtime(void);
    extern runtime_t *cpp_create_runtime(void);

    lang_register_runtime(rust_create_runtime());
    lang_register_runtime(py_create_runtime());
    lang_register_runtime(cpp_create_runtime());

    /* List available runtimes */
    runtime_t runtimes[8];
    int count = lang_list_runtimes(runtimes, 8);

    printf("Available Language Runtimes:\n");
    for (int i = 0; i < count; i++) {
        printf("  %s v%s (%s)\n", runtimes[i].name, runtimes[i].version,
               lang_type_to_string(runtimes[i].language));
    }
    printf("\n");

    /* Demonstrate language detection */
    const char *test_files[] = {
        "hello.rs", "script.py", "program.cpp", "module.c", NULL
    };

    printf("Language Detection Test:\n");
    for (int i = 0; test_files[i]; i++) {
        language_type_t lang = lang_detect_from_file(test_files[i]);
        printf("  %s -> %s\n", test_files[i], lang_type_to_string(lang));
    }
    printf("\n");

    /* Demonstrate simple Python execution */
    printf("Python Execution Test:\n");
    const char *py_code =
        "print('Hello from Python!')\n"
        "print('This is BrightS Python runtime')\n";

    if (lang_execute_string(py_code, "python", "<demo>") == 0) {
        printf("Python execution successful\n");
    } else {
        printf("Python execution failed\n");
    }
    printf("\n");

    /* Demonstrate Rust syntax checking */
    printf("Rust Syntax Check Test:\n");
    const char *rust_code =
        "fn main() {\n"
        "    println!(\"Hello from Rust!\");\n"
        "}\n";

    if (lang_execute_string(rust_code, "rust", "<demo>") == 0) {
        printf("Rust syntax check successful\n");
    } else {
        printf("Rust syntax check completed\n");
    }
    printf("\n");

    /* Demonstrate C++ syntax checking */
    printf("C++ Syntax Check Test:\n");
    const char *cpp_code =
        "#include <iostream>\n"
        "int main() {\n"
        "    std::cout << \"Hello from C++!\" << std::endl;\n"
        "    return 0;\n"
        "}\n";

    if (lang_execute_string(cpp_code, "cpp", "<demo>") == 0) {
        printf("C++ syntax check successful\n");
    } else {
        printf("C++ syntax check completed\n");
    }
    printf("\n");

    printf("Multi-language runtime demo completed!\n");
    printf("Use 'rustc', 'python', 'g++' commands for full compilation.\n");

    return 0;
}