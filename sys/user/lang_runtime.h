#ifndef BRIGHTS_LANG_RUNTIME_H
#define BRIGHTS_LANG_RUNTIME_H

#include <stdint.h>

#define MAX_LANGUAGES 8
#define MAX_RUNTIME_NAME 32
#define MAX_RUNTIME_VERSION 16

/* Supported language types */
typedef enum {
    LANG_RUST = 0,
    LANG_PYTHON = 1,
    LANG_CPP = 2,
    LANG_C = 3,
    LANG_UNKNOWN = 255
} language_type_t;

/* Runtime capabilities */
typedef enum {
    RT_CAP_COMPILE = 1 << 0,      /* Can compile source code */
    RT_CAP_INTERPRET = 1 << 1,    /* Can interpret code */
    RT_CAP_JIT = 1 << 2,          /* Has JIT compilation */
    RT_CAP_GC = 1 << 3,           /* Has garbage collection */
    RT_CAP_THREADS = 1 << 4,      /* Supports threading */
    RT_CAP_NETWORK = 1 << 5,      /* Network support */
    RT_CAP_FILESYSTEM = 1 << 6,   /* Filesystem access */
} runtime_capability_t;

/* Runtime instance */
typedef struct {
    char name[MAX_RUNTIME_NAME];
    char version[MAX_RUNTIME_VERSION];
    language_type_t language;
    uint32_t capabilities;
    void *context;                 /* Language-specific context */

    /* Function pointers for runtime operations */
    int (*init)(void *context);
    int (*shutdown)(void *context);
    int (*execute_file)(const char *filename, char **argv, char **envp);
    int (*execute_string)(const char *code, const char *filename);
    int (*compile_file)(const char *input, const char *output);
    int (*get_error)(char *buf, uint64_t size);

    /* Memory management */
    void *(*alloc)(uint64_t size);
    void (*free)(void *ptr);
    void *(*realloc)(void *ptr, uint64_t new_size);
} runtime_t;

/* Language runtime manager */
typedef struct {
    runtime_t runtimes[MAX_LANGUAGES];
    int runtime_count;
    runtime_t *current_runtime;
} language_manager_t;

/* Internal helpers (used by runtime implementations) */
int runtime_init_common(const char *name, void *context, size_t context_size,
                       size_t heap_size, void **heap_base, void **heap_ptr);
void runtime_cleanup_common(const char *name, void *heap_base, void *stack_base);

/* Public API */
int lang_init(void);
int lang_register_runtime(const runtime_t *runtime);
int lang_switch_runtime(const char *name);
runtime_t *lang_get_current_runtime(void);
runtime_t *lang_find_runtime(language_type_t lang);
int lang_execute_file(const char *filename, char **argv, char **envp);
int lang_execute_string(const char *code, const char *lang, const char *filename);
int lang_list_runtimes(runtime_t *list, int max_count);

/* Language detection */
language_type_t lang_detect_from_file(const char *filename);
language_type_t lang_detect_from_extension(const char *extension);
const char *lang_type_to_string(language_type_t type);

#endif