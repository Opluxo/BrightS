#ifndef BRIGHTS_SIMD_H
#define BRIGHTS_SIMD_H

#include <stdint.h>

/*
 * BrightS SIMD (Single Instruction, Multiple Data) Optimization Library
 *
 * Provides SIMD-accelerated operations for:
 * - Memory operations (copy, set, compare)
 * - Mathematical operations (vector add, multiply)
 * - String operations (search, compare)
 * - Cryptographic operations (hash, checksum)
 */

#ifdef __SSE2__
#include <emmintrin.h>
#endif

#ifdef __AVX__
#include <immintrin.h>
#endif

/* SIMD helper macros */
#define SIMD_AVX_CHECK(n, vec_size) (brights_simd_caps.has_avx && (n) >= (vec_size))
#define SIMD_SSE2_CHECK(n, vec_size) (brights_simd_caps.has_sse2 && (n) >= (vec_size))

/* Vector processing macros */
#define SIMD_VEC_PROCESS_AVX(type, load, store, op, dst, a, b, vec_size, total) \
    do { \
        size_t blocks = (total) / (vec_size); \
        for (size_t i = 0; i < blocks; i++) { \
            type va = load((a) + i * (vec_size)); \
            type vb = load((b) + i * (vec_size)); \
            type result = op(va, vb); \
            store((dst) + i * (vec_size), result); \
        } \
        (total) %= (vec_size); \
    } while(0)

/* SIMD capability detection */
typedef struct {
    int has_sse2;
    int has_sse4_1;
    int has_avx;
    int has_avx2;
    int has_avx512;
} simd_caps_t;

extern simd_caps_t brights_simd_caps;

/* Initialize SIMD capabilities */
void brights_simd_init(void);

/* Memory operations */
void *brights_simd_memcpy(void *dst, const void *src, size_t n);
void *brights_simd_memset(void *dst, int c, size_t n);
int brights_simd_memcmp(const void *a, const void *b, size_t n);

/* Vector operations */
void brights_simd_vec_add_f32(float *dst, const float *a, const float *b, size_t n);
void brights_simd_vec_mul_f32(float *dst, const float *a, const float *b, size_t n);
void brights_simd_vec_add_i32(int32_t *dst, const int32_t *a, const int32_t *b, size_t n);

/* String operations */
const char *brights_simd_strstr(const char *haystack, const char *needle);
size_t brights_simd_strlen(const char *str);

/* Cryptographic operations */
uint32_t brights_simd_crc32(const void *data, size_t len);
void brights_simd_md5(const void *data, size_t len, uint8_t hash[16]);

/* Parallel processing */
typedef struct {
    void *(*worker_func)(void *arg);
    void *arg;
    void *result;
} brights_parallel_task_t;

int brights_parallel_execute(brights_parallel_task_t *tasks, int num_tasks, int max_threads);

/* Performance monitoring */
typedef struct {
    uint64_t instructions_retired;
    uint64_t cache_misses;
    uint64_t branch_misses;
    uint64_t cycles;
} brights_perf_counters_t;

int brights_perf_start_monitoring(void);
int brights_perf_stop_monitoring(brights_perf_counters_t *counters);
int brights_perf_get_counters(brights_perf_counters_t *counters);

#endif /* BRIGHTS_SIMD_H */