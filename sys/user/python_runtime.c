#include "python_runtime.h"
#include "lang_runtime.h"
#include "libc.h"

static py_vm_t py_vm;

/*
 * Initialize Python runtime
 */
int py_runtime_init(void)
{
    /* Use common initialization */
    if (runtime_init_common("python", &py_vm, sizeof(py_vm),
                           PY_HEAP_SIZE, &py_vm.heap_base, &py_vm.heap_ptr) != 0) {
        return -1;
    }

    /* Initialize globals */
    py_vm.globals = calloc(256, sizeof(py_object_t *));
    if (!py_vm.globals) {
        free(py_vm.heap_base);
        printf("python: failed to allocate globals\n");
        return -1;
    }

    py_vm.builtins = calloc(256, sizeof(py_object_t *));
    if (!py_vm.builtins) {
        free(py_vm.globals);
        free(py_vm.heap_base);
        printf("python: failed to allocate builtins\n");
        return -1;
    }

    /* Setup built-ins */
    py_vm.builtins['p' - 'a'] = py_new_object(PY_FUNCTION); /* print */
    py_vm.builtins['l' - 'a'] = py_new_object(PY_FUNCTION); /* len */
    py_vm.builtins['t' - 'a'] = py_new_object(PY_FUNCTION); /* type */

    py_vm.running = 1;
    py_vm.error = 0;
    return 0;
}

/*
 * Execute Python file
 */
int py_runtime_execute_file(const char *filename, char **argv, char **envp)
{
    /* Read file */
    int fd = sys_open(filename, 0); /* O_RDONLY */
    if (fd < 0) {
        printf("python: cannot open file %s\n", filename);
        return -1;
    }

    char code[64 * 1024]; /* 64KB limit */
    int64_t n = sys_read(fd, code, sizeof(code) - 1);
    sys_close(fd);

    if (n <= 0) {
        printf("python: cannot read file\n");
        return -1;
    }
    code[n] = 0;

    return py_runtime_execute_string(code, filename);
}

/*
 * Execute a single line of Python code
 */
static int py_execute_line(const char *line)
{
    /* Very simplified execution */

    if (strstr(line, "print(")) {
        /* Handle print statements */
        const char *start = strchr(line, '(');
        if (start) {
            start++; /* Skip ( */
            const char *end = strchr(start, ')');
            if (end) {
                int len = end - start;
                char content[256];
                memcpy(content, start, len);
                content[len] = 0;

                /* Remove quotes if present */
                if (content[0] == '"' && content[len - 1] == '"') {
                    content[len - 1] = 0;
                    printf("%s\n", content + 1);
                } else if (content[0] == '\'' && content[len - 1] == '\'') {
                    content[len - 1] = 0;
                    printf("%s\n", content + 1);
                } else {
                    printf("%s\n", content);
                }
            }
        }
    } else if (strstr(line, "=")) {
        /* Variable assignment */
        printf("python: variable assignment not implemented\n");
    } else if (strstr(line, "if ")) {
        /* Conditional */
        printf("python: conditionals not implemented\n");
    } else if (strstr(line, "for ") || strstr(line, "while ")) {
        /* Loops */
        printf("python: loops not implemented\n");
    } else if (strstr(line, "def ")) {
        /* Function definition */
        printf("python: functions not implemented\n");
    } else if (strlen(line) > 0) {
        /* Expression */
        printf("python: expression evaluation not implemented: %s\n", line);
    }

    return 0;
}

/*
 * Execute Python code string
 */
int py_runtime_execute_string(const char *code, const char *filename)
{
    printf("python: executing %s\n", filename ? filename : "<string>");

    /* Very basic Python interpreter */
    /* Parse and execute line by line */

    char line[1024];
    const char *p = code;

    while (*p) {
        /* Read line */
        int i = 0;
        while (*p && *p != '\n' && i < sizeof(line) - 1) {
            line[i++] = *p++;
        }
        if (*p == '\n') p++;
        line[i] = 0;

        /* Skip empty lines and comments */
        if (i == 0 || line[0] == '#') continue;

        /* Execute line */
        if (py_execute_line(line) != 0) {
            printf("python: execution error\n");
            return -1;
        }
    }

    return 0;
}

/*
 * Create new Python object
 */
py_object_t *py_new_object(py_object_type_t type)
{
    if (py_vm.object_count >= PY_MAX_OBJECTS) {
        printf("python: object limit reached\n");
        return NULL;
    }

    py_object_t *obj = malloc(sizeof(py_object_t));
    if (!obj) return NULL;

    memset(obj, 0, sizeof(py_object_t));
    obj->type = type;
    obj->ref_count = 1;

    py_vm.objects[py_vm.object_count++] = obj;
    return obj;
}

/*
 * Delete Python object
 */
void py_delete_object(py_object_t *obj)
{
    if (!obj) return;

    obj->ref_count--;
    if (obj->ref_count > 0) return;

    /* Free object-specific data */
    switch (obj->type) {
    case PY_STR:
        if (obj->value.str_val.data) free(obj->value.str_val.data);
        break;
    case PY_LIST:
        if (obj->value.list_val.items) free(obj->value.list_val.items);
        break;
    case PY_DICT:
        if (obj->value.dict_val.keys) free(obj->value.dict_val.keys);
        if (obj->value.dict_val.values) free(obj->value.dict_val.values);
        break;
    default:
        break;
    }

    free(obj);

    /* Remove from VM object list */
    for (int i = 0; i < py_vm.object_count; i++) {
        if (py_vm.objects[i] == obj) {
            py_vm.objects[i] = py_vm.objects[--py_vm.object_count];
            break;
        }
    }
}

/*
 * Create common objects
 */
py_object_t *py_none(void)
{
    static py_object_t *none_obj = NULL;
    if (!none_obj) {
        none_obj = py_new_object(PY_NONE);
    }
    return none_obj;
}

py_object_t *py_bool(int val)
{
    py_object_t *obj = py_new_object(PY_BOOL);
    if (obj) {
        obj->value.int_val = val ? 1 : 0;
    }
    return obj;
}

py_object_t *py_int(int64_t val)
{
    py_object_t *obj = py_new_object(PY_INT);
    if (obj) {
        obj->value.int_val = val;
    }
    return obj;
}

py_object_t *py_float(double val)
{
    py_object_t *obj = py_new_object(PY_FLOAT);
    if (obj) {
        obj->value.float_val = val;
    }
    return obj;
}

py_object_t *py_str(const char *str)
{
    py_object_t *obj = py_new_object(PY_STR);
    if (obj) {
        obj->value.str_val.len = strlen(str);
        obj->value.str_val.data = malloc(obj->value.str_val.len + 1);
        if (obj->value.str_val.data) {
            strcpy(obj->value.str_val.data, str);
        }
    }
    return obj;
}

py_object_t *py_list(void)
{
    py_object_t *obj = py_new_object(PY_LIST);
    if (obj) {
        obj->value.list_val.capacity = 8;
        obj->value.list_val.items = calloc(obj->value.list_val.capacity, sizeof(py_object_t *));
    }
    return obj;
}

/*
 * Built-in functions
 */
py_object_t *py_builtin_print(py_object_t **args, int argc)
{
    for (int i = 0; i < argc; i++) {
        switch (args[i]->type) {
        case PY_STR:
            printf("%s", args[i]->value.str_val.data);
            break;
        case PY_INT:
            printf("%lld", args[i]->value.int_val);
            break;
        case PY_BOOL:
            printf("%s", args[i]->value.int_val ? "True" : "False");
            break;
        default:
            printf("<object>");
            break;
        }
        if (i < argc - 1) printf(" ");
    }
    printf("\n");
    return py_none();
}

py_object_t *py_builtin_len(py_object_t **args, int argc)
{
    if (argc != 1) return py_none();

    py_object_t *obj = args[0];
    int64_t len = 0;

    switch (obj->type) {
    case PY_STR:
        len = obj->value.str_val.len;
        break;
    case PY_LIST:
        len = obj->value.list_val.len;
        break;
    case PY_DICT:
        len = obj->value.dict_val.len;
        break;
    default:
        return py_none();
    }

    return py_int(len);
}

py_object_t *py_builtin_type(py_object_t **args, int argc)
{
    if (argc != 1) return py_none();

    /* Return type name as string */
    const char *type_name = "unknown";
    switch (args[0]->type) {
    case PY_NONE: type_name = "NoneType"; break;
    case PY_BOOL: type_name = "bool"; break;
    case PY_INT: type_name = "int"; break;
    case PY_FLOAT: type_name = "float"; break;
    case PY_STR: type_name = "str"; break;
    case PY_LIST: type_name = "list"; break;
    case PY_DICT: type_name = "dict"; break;
    default: break;
    }

    return py_str(type_name);
}

/*
 * Create the Python runtime instance for the language manager
 */
runtime_t *py_create_runtime(void)
{
    static runtime_t py_rt;

    memset(&py_rt, 0, sizeof(runtime_t));
    strcpy(py_rt.name, "python");
    strcpy(py_rt.version, "3.0.0");
    py_rt.language = LANG_PYTHON;
    py_rt.capabilities = RT_CAP_INTERPRET | RT_CAP_GC | RT_CAP_NETWORK;

    py_rt.init = (int (*)(void *))py_runtime_init;
    py_rt.execute_file = py_runtime_execute_file;
    py_rt.execute_string = py_runtime_execute_string;

    return &py_rt;
}