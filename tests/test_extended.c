// Test SIMD functionality
static void test_simd_functions(void) {
    printf("Testing SIMD functions...\n");

    // Test memory operations
    char src[64] = "Hello SIMD World!";
    char dst[64];

    brights_simd_memcpy(dst, src, strlen(src) + 1);
    assert(strcmp(dst, src) == 0);

    brights_simd_memset(dst, 'X', 10);
    for (int i = 0; i < 10; i++) {
        assert(dst[i] == 'X');
    }

    printf("SIMD memory operations: PASSED\n");

    // Test vector operations (if available)
    if (brights_simd_caps.has_sse2) {
        float a[4] = {1.0f, 2.0f, 3.0f, 4.0f};
        float b[4] = {0.5f, 1.5f, 2.5f, 3.5f};
        float result[4];

        brights_simd_vec_add_f32(result, a, b, 4);
        assert(result[0] == 1.5f && result[1] == 3.5f);

        printf("SIMD vector operations: PASSED\n");
    } else {
        printf("SIMD vector operations: SKIPPED (no SSE2)\n");
    }
}

// Test cache functionality
static void test_cache_functions(void) {
    printf("Testing cache functions...\n");

    cache_config_t config = {
        .name = "test_cache",
        .max_size = 1024,
        .max_entries = 10,
        .ttl_seconds = 60,
        .cleanup_func = NULL
    };

    cache_t *cache;
    assert(cache_create(&config, &cache) == 0);

    // Test put/get
    const char *test_data = "Hello Cache!";
    assert(cache_put(cache, 12345, test_data, strlen(test_data) + 1) == 0);

    char *retrieved_data;
    size_t data_size;
    assert(cache_get(cache, 12345, (void**)&retrieved_data, &data_size) == 0);
    assert(strcmp(retrieved_data, test_data) == 0);

    // Test contains
    assert(cache_contains(cache, 12345) == 1);
    assert(cache_contains(cache, 99999) == 0);

    // Test remove
    assert(cache_remove(cache, 12345) == 0);
    assert(cache_contains(cache, 12345) == 0);

    cache_destroy(cache);
    printf("Cache operations: PASSED\n");
}

// Test monitoring functionality
static void test_monitor_functions(void) {
    printf("Testing monitoring functions...\n");

    assert(brights_monitor_init() == 0);

    // Test health monitoring
    const system_health_t *health = brights_monitor_get_health();
    assert(health != NULL);
    assert(health->overall_health >= 0 && health->overall_health <= 100);

    // Test performance stats
    performance_stats_t stats;
    assert(brights_monitor_get_performance_stats(&stats) == 0);
    assert(stats.process_count > 0);

    printf("Monitoring functions: PASSED\n");
}

// Extended test suite
void run_extended_tests(void) {
    printf("Running extended test suite...\n");

    test_simd_functions();
    test_cache_functions();
    test_monitor_functions();

    printf("All extended tests passed!\n");
}