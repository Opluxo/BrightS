#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>

#include "../include/kernel/simd.h"
#include "../include/kernel/cache.h"

static void test_simd_memops(void)
{
  printf("Testing SIMD memory operations...\n");

  char src[64] = "Hello SIMD World!";
  char dst[64];

  brights_simd_memcpy(dst, src, strlen(src) + 1);
  assert(strcmp(dst, src) == 0);

  brights_simd_memset(dst, 'X', 10);
  for (int i = 0; i < 10; i++) {
    assert(dst[i] == 'X');
  }

  printf("SIMD memory operations: PASSED\n");
}

static void test_simd_vec(void)
{
  printf("Testing SIMD vector operations...\n");

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

static void test_cache_basic(void)
{
  printf("Testing cache functions...\n");

  cache_config_t config = {
    .name = "test_cache",
    .max_size = 1024,
    .max_entries = 10,
    .ttl_seconds = 60,
    .cleanup_func = NULL
  };

  cache_t *cache = NULL;
  assert(cache_create(&config, &cache) == 0);
  assert(cache != NULL);

  const char *test_data = "Hello Cache!";
  assert(cache_put(cache, 12345, test_data, strlen(test_data) + 1) == 0);

  char *retrieved_data = NULL;
  size_t data_size = 0;
  assert(cache_get(cache, 12345, (void**)&retrieved_data, &data_size) == 0);
  assert(strcmp(retrieved_data, test_data) == 0);

  assert(cache_contains(cache, 12345) == 1);
  assert(cache_contains(cache, 99999) == 0);

  assert(cache_remove(cache, 12345) == 0);
  assert(cache_contains(cache, 12345) == 0);

  cache_destroy(cache);

  printf("Cache operations: PASSED\n");
}

void run_extended_tests(void)
{
  printf("Running extended test suite...\n");

  test_simd_memops();
  test_simd_vec();
  test_cache_basic();

  printf("All extended tests passed!\n");
}

int main(void)
{
  run_extended_tests();
  return 0;
}
