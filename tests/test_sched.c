#include <assert.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>

/* Simulate scheduler slot management for testing */
#define TEST_MAX_PROC 64

static int test_proc_slots[TEST_MAX_PROC];
static int test_proc_count = 0;

static int test_slot_alloc(void)
{
  for (int i = 0; i < TEST_MAX_PROC; ++i) {
    if (test_proc_slots[i] == 0) {
      test_proc_slots[i] = 1;
      test_proc_count++;
      return i;
    }
  }
  return -1;
}

static void test_slot_free(int slot)
{
  if (slot < 0 || slot >= TEST_MAX_PROC) return;
  if (test_proc_slots[slot]) {
    test_proc_slots[slot] = 0;
    test_proc_count--;
  }
}

static void test_slot_alloc_basic(void)
{
  memset(test_proc_slots, 0, sizeof(test_proc_slots));
  test_proc_count = 0;

  int s1 = test_slot_alloc();
  assert(s1 >= 0);
  assert(test_proc_count == 1);

  int s2 = test_slot_alloc();
  assert(s2 >= 0);
  assert(s2 != s1);
  assert(test_proc_count == 2);

  test_slot_free(s1);
  assert(test_proc_count == 1);

  test_slot_free(s2);
  assert(test_proc_count == 0);
}

static void test_slot_alloc_exhaust(void)
{
  memset(test_proc_slots, 0, sizeof(test_proc_slots));
  test_proc_count = 0;

  int slots[TEST_MAX_PROC];
  int count = 0;
  for (int i = 0; i < TEST_MAX_PROC + 10; ++i) {
    int s = test_slot_alloc();
    if (s >= 0) slots[count++] = s;
  }
  assert(count == TEST_MAX_PROC);
  assert(test_slot_alloc() == -1);

  for (int i = 0; i < count; ++i) {
    test_slot_free(slots[i]);
  }
  assert(test_proc_count == 0);
}

static void test_slot_double_free(void)
{
  memset(test_proc_slots, 0, sizeof(test_proc_slots));
  test_proc_count = 0;

  int s = test_slot_alloc();
  assert(s >= 0);
  assert(test_proc_count == 1);

  test_slot_free(s);
  assert(test_proc_count == 0);

  test_slot_free(s);
  assert(test_proc_count == 0);
}

static void test_slot_invalid_free(void)
{
  memset(test_proc_slots, 0, sizeof(test_proc_slots));
  test_proc_count = 0;

  test_slot_free(-1);
  test_slot_free(TEST_MAX_PROC);
  assert(test_proc_count == 0);
}

int main(void)
{
  printf("test_sched: running slot allocation tests...\n");

  test_slot_alloc_basic();
  printf("  PASS: basic alloc/free\n");

  test_slot_alloc_exhaust();
  printf("  PASS: exhaust all slots\n");

  test_slot_double_free();
  printf("  PASS: double free safety\n");

  test_slot_invalid_free();
  printf("  PASS: invalid free safety\n");

  printf("test_sched: ALL TESTS PASSED\n");
  return 0;
}
