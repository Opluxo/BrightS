#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "../sys/kernel/fs/ramfs.h"
#include "../sys/kernel/core/kernel_util.h"

/* Simple timing utilities for host system */
static inline uint64_t benchmark_start(void)
{
    // Simple timing using loop counter (not accurate but sufficient for testing)
    return 0;
}

static inline uint64_t benchmark_end(uint64_t start)
{
    (void)start;
    return 1; // Placeholder
}

static void benchmark_memory_allocation(void)
{
    // Skip memory allocation benchmark in host environment
    // kmalloc/kfree are not available in test environment
}

static void benchmark_string_operations(void)
{
    const char *test_str = "The quick brown fox jumps over the lazy dog";
    const char *needle = "fox";
    const int iterations = 10000;

    // Simple correctness test instead of timing
    for (int i = 0; i < iterations; i++) {
        char *result = kutil_strstr(test_str, needle);
        assert(result != NULL);
        assert(kutil_strcmp(result, "fox jumps over the lazy dog") == 0);
    }
}

static void benchmark_file_operations(void)
{
    brights_ramfs_init();

    // Create test file
    assert(brights_ramfs_create("/bench_test.txt") >= 0);
    int fd = brights_ramfs_open("/bench_test.txt");
    assert(fd >= 0);

    const int data_size = 128;
    uint8_t data[data_size];
    for (int i = 0; i < data_size; i++) {
        data[i] = (uint8_t)(i % 256);
    }

    const int iterations = 5;

    // Simple write and read test
    assert(brights_ramfs_write(fd, 0, data, data_size) == data_size);

    uint8_t read_buf[data_size];
    assert(brights_ramfs_read(fd, 0, read_buf, data_size) == data_size);

    // Verify data
    for (int j = 0; j < data_size; j++) {
        assert(read_buf[j] == data[j]);
    }

    assert(brights_ramfs_file_size(fd) == data_size);

    brights_ramfs_close(fd);
    brights_ramfs_unlink("/bench_test.txt");
}

static void benchmark_memory_functions(void)
{
    const int buffer_size = 1024;
    uint8_t src[buffer_size];
    uint8_t dst[buffer_size];

    // Initialize source buffer
    for (int i = 0; i < buffer_size; i++) {
        src[i] = (uint8_t)i;
    }

    const int iterations = 100;

    // Test memcpy correctness
    for (int i = 0; i < iterations; i++) {
        memcpy(dst, src, buffer_size);
        for (int j = 0; j < buffer_size; j++) {
            assert(dst[j] == src[j]);
        }
    }

    // Test memset
    for (int i = 0; i < iterations; i++) {
        memset(dst, 0xAA, buffer_size);
        for (int j = 0; j < buffer_size; j++) {
            assert(dst[j] == 0xAA);
        }
    }

    // Test memcmp
    for (int i = 0; i < iterations; i++) {
        memset(dst, 0xAA, buffer_size);
        assert(memcmp(src, dst, buffer_size) != 0);
        memcpy(dst, src, buffer_size);
        assert(memcmp(src, dst, buffer_size) == 0);
    }
}

int main(void)
{
    benchmark_memory_allocation();
    benchmark_string_operations();
    benchmark_file_operations();
    benchmark_memory_functions();

    return 0;
}