#include "rust_runtime.h"
#include "lang_runtime.h"
#include "libc.h"

static rust_runtime_context_t rust_ctx;

/* Default panic handler */
static void rust_default_panic_handler(rust_panic_info_t *info)
{
    printf("Rust panic: %s at %s:%d:%d\n",
           info->message, info->file, info->line, info->column);
    sys_exit(101); /* Rust panic exit code */
}

/*
 * Initialize Rust runtime
 */
int rust_runtime_init(void)
{
    /* Use common initialization */
    if (runtime_init_common("rust", &rust_ctx, sizeof(rust_ctx),
                           RUST_HEAP_SIZE, &rust_ctx.heap_base, NULL) != 0) {
        return -1;
    }

    /* Initialize stack separately */
    rust_ctx.stack_size = RUST_STACK_SIZE;
    rust_ctx.stack_base = malloc(rust_ctx.stack_size);
    if (!rust_ctx.stack_base) {
        printf("rust: failed to allocate stack\n");
        free(rust_ctx.heap_base);
        return -1;
    }

    /* Setup memory management */
    rust_ctx.alloc = rust_alloc;
    rust_ctx.dealloc = rust_dealloc;
    rust_ctx.realloc = rust_realloc;

    /* Setup panic handler */
    rust_ctx.panic_handler = rust_default_panic_handler;

    printf("rust: runtime initialized (heap: %dKB, stack: %dKB)\n",
           rust_ctx.heap_size / 1024, rust_ctx.stack_size / 1024);
    return 0;
}

/*
 * Execute a Rust file (simplified - just compile and run)
 */
int rust_runtime_execute_file(const char *filename, char **argv, char **envp)
{
    /* Save command line arguments */
    rust_ctx.argc = 0;
    if (argv) {
        while (argv[rust_ctx.argc]) rust_ctx.argc++;
    }
    rust_ctx.argv = argv;
    rust_ctx.envp = envp;

    /* For now, we don't actually compile Rust code */
    /* This would require a full Rust compiler implementation */
    printf("rust: file execution not implemented yet: %s\n", filename);
    printf("rust: use 'rustc' command to compile, then execute binary\n");

    return -1;
}

/*
 * Execute Rust code string (simplified interpreter)
 */
int rust_runtime_execute_string(const char *code, const char *filename)
{
    printf("rust: executing code from %s\n", filename ? filename : "<string>");
    printf("rust: code interpretation not fully implemented\n");
    printf("rust: basic syntax check...\n");

    /* Very basic syntax validation */
    if (strstr(code, "fn main()")) {
        printf("rust: found main function\n");
    }

    if (strstr(code, "println!")) {
        printf("rust: found println! macro\n");
    }

    printf("rust: execution would require full compiler\n");
    return -1;
}

/*
 * Trigger a Rust panic
 */
int rust_panic(rust_panic_info_t *info)
{
    if (rust_ctx.panic_handler) {
        rust_ctx.panic_handler(info);
    }
    return -1;
}

/*
 * Rust-compatible memory allocation
 */
void *rust_alloc(uint64_t size, uint64_t align)
{
    /* For now, just use our malloc */
    /* Real Rust would use jemalloc or similar */
    return malloc(size);
}

void rust_dealloc(void *ptr, uint64_t size, uint64_t align)
{
    (void)size; (void)align; /* Unused in our simple implementation */
    free(ptr);
}

void *rust_realloc(void *ptr, uint64_t old_size, uint64_t new_size, uint64_t align)
{
    (void)old_size; (void)align; /* Unused in our simple implementation */
    return realloc(ptr, new_size);
}

/*
 * Rust string operations
 */
void rust_println(rust_str_t s)
{
    /* Write to stdout */
    sys_write(1, s.data, s.len);
    sys_write(1, "\n", 1);
}

rust_str_t rust_str_from_cstr(const char *cstr)
{
    rust_str_t str;
    str.data = cstr;
    str.len = strlen(cstr);
    return str;
}

char *rust_str_to_cstr(rust_str_t s)
{
    /* Create a null-terminated copy */
    char *cstr = malloc(s.len + 1);
    if (cstr) {
        memcpy(cstr, s.data, s.len);
        cstr[s.len] = 0;
    }
    return cstr;
}

int rust_str_eq(rust_str_t a, rust_str_t b)
{
    if (a.len != b.len) return 0;
    return memcmp(a.data, b.data, a.len) == 0;
}

/*
 * Create the Rust runtime instance for the language manager
 */
runtime_t *rust_create_runtime(void)
{
    static runtime_t rust_rt;

    memset(&rust_rt, 0, sizeof(runtime_t));
    strcpy(rust_rt.name, "rust");
    strcpy(rust_rt.version, "1.0.0");
    rust_rt.language = LANG_RUST;
    rust_rt.capabilities = RT_CAP_COMPILE | RT_CAP_THREADS | RT_CAP_FILESYSTEM;

    rust_rt.init = (int (*)(void *))rust_runtime_init;
    rust_rt.execute_file = rust_runtime_execute_file;
    rust_rt.execute_string = rust_runtime_execute_string;

    return &rust_rt;
}