#ifndef BRIGHTS_RUST_RUNTIME_H
#define BRIGHTS_RUST_RUNTIME_H

#include "lang_runtime.h"
#include <stdint.h>

/* Rust-specific types and constants */
#define RUST_STACK_SIZE (64 * 1024)  /* 64KB stack per thread */
#define RUST_HEAP_SIZE (1024 * 1024) /* 1MB initial heap */

/* Rust panic info */
typedef struct {
    const char *message;
    const char *file;
    uint32_t line;
    uint32_t column;
} rust_panic_info_t;

/* Rust string (fat pointer) */
typedef struct {
    const char *data;
    uint64_t len;
} rust_str_t;

/* Rust vector (simplified) */
typedef struct {
    void *data;
    uint64_t len;
    uint64_t capacity;
} rust_vec_t;

/* Rust Option type */
typedef enum { RUST_NONE, RUST_SOME } rust_option_tag_t;

typedef struct {
    rust_option_tag_t tag;
    union {
        void *some;
    } value;
} rust_option_t;

/* Rust Result type */
typedef enum { RUST_OK, RUST_ERR } rust_result_tag_t;

typedef struct {
    rust_result_tag_t tag;
    union {
        void *ok;
        void *err;
    } value;
} rust_result_t;

/* Function pointers for Rust runtime */
typedef void (*rust_panic_handler_t)(rust_panic_info_t *info);
typedef void *(*rust_alloc_handler_t)(uint64_t size, uint64_t align);
typedef void (*rust_dealloc_handler_t)(void *ptr, uint64_t size, uint64_t align);
typedef void *(*rust_realloc_handler_t)(void *ptr, uint64_t old_size, uint64_t new_size, uint64_t align);

/* Rust runtime context */
typedef struct {
    /* Memory management */
    rust_alloc_handler_t alloc;
    rust_dealloc_handler_t dealloc;
    rust_realloc_handler_t realloc;

    /* Panic handling */
    rust_panic_handler_t panic_handler;

    /* Stack and heap */
    void *stack_base;
    uint64_t stack_size;
    void *heap_base;
    uint64_t heap_size;

    /* Command line arguments */
    int argc;
    char **argv;
    char **envp;
} rust_runtime_context_t;

/* Public API */
int rust_runtime_init(void);
int rust_runtime_execute_file(const char *filename, char **argv, char **envp);
int rust_runtime_execute_string(const char *code, const char *filename);
int rust_panic(rust_panic_info_t *info);

/* Memory management (Rust-compatible) */
void *rust_alloc(uint64_t size, uint64_t align);
void rust_dealloc(void *ptr, uint64_t size, uint64_t align);
void *rust_realloc(void *ptr, uint64_t old_size, uint64_t new_size, uint64_t align);

/* Standard library functions */
void rust_println(rust_str_t s);
rust_str_t rust_str_from_cstr(const char *cstr);
char *rust_str_to_cstr(rust_str_t s);
int rust_str_eq(rust_str_t a, rust_str_t b);

#endif