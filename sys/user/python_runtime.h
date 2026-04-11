#ifndef BRIGHTS_PYTHON_RUNTIME_H
#define BRIGHTS_PYTHON_RUNTIME_H

#include "lang_runtime.h"
#include <stdint.h>

#define PY_STACK_SIZE (128 * 1024)  /* 128KB Python stack */
#define PY_HEAP_SIZE (2 * 1024 * 1024) /* 2MB Python heap */
#define PY_MAX_OBJECTS 1024

/* Python object types */
typedef enum {
    PY_NONE = 0,
    PY_BOOL,
    PY_INT,
    PY_FLOAT,
    PY_STR,
    PY_LIST,
    PY_DICT,
    PY_CODE,
    PY_FUNCTION,
    PY_MODULE
} py_object_type_t;

/* Python object */
typedef struct py_object {
    py_object_type_t type;
    uint32_t ref_count;
    union {
        int64_t int_val;
        double float_val;
        struct {
            char *data;
            uint64_t len;
        } str_val;
        struct {
            struct py_object **items;
            uint64_t len;
            uint64_t capacity;
        } list_val;
        struct {
            struct py_object **keys;
            struct py_object **values;
            uint64_t len;
            uint64_t capacity;
        } dict_val;
        struct {
            uint8_t *code;
            uint64_t code_len;
            struct py_object *consts;
            struct py_object *names;
            uint64_t stack_size;
        } code_val;
    } value;
} py_object_t;

/* Python frame (call stack) */
typedef struct py_frame {
    struct py_frame *back;
    struct py_object *code;
    uint64_t pc;              /* Program counter */
    struct py_object **locals;
    struct py_object **stack;
    uint64_t stack_top;
    uint64_t stack_size;
} py_frame_t;

/* Python virtual machine */
typedef struct {
    py_object_t *objects[PY_MAX_OBJECTS];
    uint64_t object_count;

    py_frame_t *current_frame;
    py_object_t **globals;
    py_object_t **builtins;

    /* Memory */
    void *heap_base;
    uint64_t heap_size;
    void *heap_ptr;

    /* Execution state */
    int running;
    int error;
    char error_msg[256];
} py_vm_t;

/* Python opcodes (simplified) */
typedef enum {
    OP_LOAD_CONST = 1,
    OP_LOAD_NAME,
    OP_STORE_NAME,
    OP_LOAD_GLOBAL,
    OP_STORE_GLOBAL,
    OP_POP_TOP,
    OP_RETURN_VALUE,
    OP_BINARY_ADD,
    OP_BINARY_SUBTRACT,
    OP_PRINT_ITEM,
    OP_PRINT_NEWLINE,
    OP_JUMP_FORWARD,
    OP_JUMP_IF_FALSE,
    OP_COMPARE_OP,
    OP_SETUP_LOOP,
    OP_POP_BLOCK,
    OP_MAKE_FUNCTION,
    OP_CALL_FUNCTION,
    OP_LOAD_ATTR,
    OP_STORE_ATTR
} py_opcode_t;

/* Public API */
int py_runtime_init(void);
int py_runtime_execute_file(const char *filename, char **argv, char **envp);
int py_runtime_execute_string(const char *code, const char *filename);

/* Object management */
py_object_t *py_new_object(py_object_type_t type);
void py_delete_object(py_object_t *obj);
py_object_t *py_none(void);
py_object_t *py_bool(int val);
py_object_t *py_int(int64_t val);
py_object_t *py_float(double val);
py_object_t *py_str(const char *str);
py_object_t *py_list(void);

/* Built-in functions */
py_object_t *py_builtin_print(py_object_t **args, int argc);
py_object_t *py_builtin_len(py_object_t **args, int argc);
py_object_t *py_builtin_type(py_object_t **args, int argc);

#endif