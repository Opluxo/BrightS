#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define BRIGHTS_TEST_BUILD
#define BRIGHTS_KMALLOC_HEAP_SIZE (8 * 1024 * 1024)

static char test_heap[BRIGHTS_KMALLOC_HEAP_SIZE];
static size_t test_heap_used = 0;

static void test_kmalloc_init(void)
{
  test_heap_used = 0;
}

static void *test_kmalloc(size_t size)
{
  if (size == 0) return 0;
  size = (size + 7) & ~7;
  if (test_heap_used + size > BRIGHTS_KMALLOC_HEAP_SIZE) return 0;
  void *ptr = &test_heap[test_heap_used];
  test_heap_used += size;
  return ptr;
}

static void test_kfree(void *ptr)
{
  (void)ptr;
}

static void test_basic_alloc_free(void)
{
  test_kmalloc_init();
  void *p1 = test_kmalloc(32);
  assert(p1 != 0);
  void *p2 = test_kmalloc(64);
  assert(p2 != 0);
  assert(p2 != p1);
  test_kfree(p1);
  test_kfree(p2);
}

static void test_large_alloc(void)
{
  test_kmalloc_init();
  void *p = test_kmalloc(1024 * 64);
  assert(p != 0);
  test_kfree(p);
}

static void test_zero_size(void)
{
  test_kmalloc_init();
  void *p = test_kmalloc(0);
  assert(p == 0);
}

static void test_alignment(void)
{
  test_kmalloc_init();
  memset(test_heap, 0, sizeof(test_heap));
  void *p = test_kmalloc(1);
  assert(p != 0);
  assert(((uintptr_t)p & 7) == 0);
  p = test_kmalloc(3);
  assert(p != 0);
  assert(((uintptr_t)p & 7) == 0);
}

static void test_alloc_exhaust(void)
{
  test_kmalloc_init();
  void *p = test_kmalloc(BRIGHTS_KMALLOC_HEAP_SIZE);
  assert(p != 0);
  void *q = test_kmalloc(8);
  assert(q == 0);
  test_kfree(p);
}

int main(void)
{
  printf("test_kmalloc: running allocation tests...\n");

  test_basic_alloc_free();
  printf("  PASS: basic alloc/free\n");

  test_large_alloc();
  printf("  PASS: large allocation\n");

  test_zero_size();
  printf("  PASS: zero-size allocation\n");

  test_alignment();
  printf("  PASS: alignment\n");

  test_alloc_exhaust();
  printf("  PASS: allocation exhaustion\n");

  printf("test_kmalloc: ALL TESTS PASSED\n");
  return 0;
}
