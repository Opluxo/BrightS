#ifndef BRIGHTS_CPP_RUNTIME_H
#define BRIGHTS_CPP_RUNTIME_H

#include "lang_runtime.h"
#include <stdint.h>

/* C++ specific types and constants */
#define CPP_STACK_SIZE (256 * 1024)  /* 256KB C++ stack */
#define CPP_HEAP_SIZE (4 * 1024 * 1024) /* 4MB C++ heap */

/* C++ exception handling (simplified) */
typedef struct {
    const char *message;
    const char *type;
    void *throw_addr;
    void *catch_addr;
} cpp_exception_t;

/* C++ runtime context */
typedef struct {
    /* Memory management */
    void *heap_base;
    uint64_t heap_size;
    void *heap_ptr;

    /* Exception handling */
    cpp_exception_t current_exception;
    int exception_pending;

    /* RTTI support (simplified) */
    void **vtable_cache;
    int vtable_count;

    /* Standard library objects */
    void *iostream_cout;
    void *iostream_cin;
    void *iostream_cerr;

    /* Command line arguments */
    int argc;
    char **argv;
} cpp_runtime_context_t;

/* C++ standard library functions */
void cpp_cout_write(const char *str);
char *cpp_cin_read(void);
void cpp_cerr_write(const char *str);

/* Exception handling */
void cpp_throw_exception(const char *type, const char *message);
int cpp_has_pending_exception(void);
cpp_exception_t *cpp_get_current_exception(void);
void cpp_clear_exception(void);

/* RTTI support */
const char *cpp_typeid_name(void *object);
int cpp_dynamic_cast(void *object, const char *target_type);

/* Standard library classes (simplified) */
typedef struct {
    char *data;
    uint64_t size;
    uint64_t capacity;
} cpp_string_t;

typedef struct {
    void **data;
    uint64_t size;
    uint64_t capacity;
} cpp_vector_t;

/* Public API */
int cpp_runtime_init(void);
int cpp_runtime_execute_file(const char *filename, char **argv, char **envp);
int cpp_runtime_execute_string(const char *code, const char *filename);

#endif