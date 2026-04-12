#include "lang_runtime.h"
#include "libc.h"

static language_manager_t lang_mgr;

/*
 * Common runtime initialization helper (public for use by runtime implementations)
 */
int runtime_init_common(const char *name, void *context, size_t context_size,
                       size_t heap_size, void **heap_base, void **heap_ptr)
{
    /* Initialize context */
    memset(context, 0, context_size);

    /* Allocate heap */
    *heap_base = malloc(heap_size);
    if (!*heap_base) {
        printf("%s: failed to allocate heap (%zu bytes)\n", name, heap_size);
        return -1;
    }

    if (heap_ptr) {
        *heap_ptr = *heap_base;
    }

    printf("%s: runtime initialized (heap: %zuKB)\n", name, heap_size / 1024);
    return 0;
}

/*
 * Common runtime cleanup helper
 */
static void runtime_cleanup_common(const char *name, void *heap_base, void *stack_base)
{
    if (heap_base) {
        free(heap_base);
    }
    if (stack_base) {
        free(stack_base);
    }
    printf("%s: runtime cleaned up\n", name);
}

/*
 * Initialize language runtime system
 */
int lang_init(void)
{
    memset(&lang_mgr, 0, sizeof(lang_mgr));
    lang_mgr.runtime_count = 0;
    lang_mgr.current_runtime = NULL;
    printf("lang: runtime system initialized\n");
    return 0;
}

/*
 * Register a new runtime
 */
int lang_register_runtime(const runtime_t *runtime)
{
    if (lang_mgr.runtime_count >= MAX_LANGUAGES) {
        printf("lang: too many runtimes registered\n");
        return -1;
    }

    memcpy(&lang_mgr.runtimes[lang_mgr.runtime_count], runtime,
           sizeof(runtime_t));
    lang_mgr.runtime_count++;

    printf("lang: registered runtime '%s' (%s)\n",
           runtime->name, lang_type_to_string(runtime->language));

    return 0;
}

/*
 * Switch to a different runtime by name
 */
int lang_switch_runtime(const char *name)
{
    for (int i = 0; i < lang_mgr.runtime_count; i++) {
        if (strcmp(lang_mgr.runtimes[i].name, name) == 0) {
            lang_mgr.current_runtime = &lang_mgr.runtimes[i];
            printf("lang: switched to runtime '%s'\n", name);
            return 0;
        }
    }

    printf("lang: runtime '%s' not found\n", name);
    return -1;
}

/*
 * Get current runtime
 */
runtime_t *lang_get_current_runtime(void)
{
    return lang_mgr.current_runtime;
}

/*
 * Find runtime by language type
 */
runtime_t *lang_find_runtime(language_type_t lang)
{
    for (int i = 0; i < lang_mgr.runtime_count; i++) {
        if (lang_mgr.runtimes[i].language == lang) {
            return &lang_mgr.runtimes[i];
        }
    }
    return NULL;
}

/*
 * Execute a file with current runtime
 */
int lang_execute_file(const char *filename, char **argv, char **envp)
{
    if (!lang_mgr.current_runtime) {
        printf("lang: no runtime selected\n");
        return -1;
    }

    if (!lang_mgr.current_runtime->execute_file) {
        printf("lang: current runtime does not support file execution\n");
        return -1;
    }

    return lang_mgr.current_runtime->execute_file(filename, argv, envp);
}

/*
 * Execute code string with specified language
 */
int lang_execute_string(const char *code, const char *lang, const char *filename)
{
    language_type_t type = LANG_UNKNOWN;

    if (strcmp(lang, "rust") == 0 || strcmp(lang, "rs") == 0) {
        type = LANG_RUST;
    } else if (strcmp(lang, "python") == 0 || strcmp(lang, "py") == 0) {
        type = LANG_PYTHON;
    } else if (strcmp(lang, "cpp") == 0 || strcmp(lang, "c++") == 0 || strcmp(lang, "cxx") == 0) {
        type = LANG_CPP;
    } else if (strcmp(lang, "c") == 0) {
        type = LANG_C;
    }

    runtime_t *rt = lang_find_runtime(type);
    if (!rt) {
        printf("lang: no runtime found for language '%s'\n", lang);
        return -1;
    }

    if (!rt->execute_string) {
        printf("lang: runtime '%s' does not support string execution\n", rt->name);
        return -1;
    }

    return rt->execute_string(code, filename);
}

/*
 * List all registered runtimes
 */
int lang_list_runtimes(runtime_t *list, int max_count)
{
    int count = lang_mgr.runtime_count < max_count ? lang_mgr.runtime_count : max_count;
    memcpy(list, lang_mgr.runtimes, count * sizeof(runtime_t));
    return count;
}

/*
 * Detect language from file extension
 */
language_type_t lang_detect_from_extension(const char *extension)
{
    if (strcmp(extension, "rs") == 0) return LANG_RUST;
    if (strcmp(extension, "py") == 0) return LANG_PYTHON;
    if (strcmp(extension, "cpp") == 0 || strcmp(extension, "cxx") == 0 || strcmp(extension, "cc") == 0) return LANG_CPP;
    if (strcmp(extension, "c") == 0 || strcmp(extension, "h") == 0) return LANG_C;
    return LANG_UNKNOWN;
}

/*
 * Detect language from filename
 */
language_type_t lang_detect_from_file(const char *filename)
{
    const char *ext = strrchr(filename, '.');
    if (ext) {
        return lang_detect_from_extension(ext + 1);
    }
    return LANG_UNKNOWN;
}

/*
 * Convert language type to string
 */
const char *lang_type_to_string(language_type_t type)
{
    switch (type) {
    case LANG_RUST: return "Rust";
    case LANG_PYTHON: return "Python";
    case LANG_CPP: return "C++";
    case LANG_C: return "C";
    default: return "Unknown";
    }
}