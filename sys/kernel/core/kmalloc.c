#include <stddef.h>
#include <stdint.h>

#define BRIGHTS_KMALLOC_HEAP_SIZE (256u * 1024u)
#define BRIGHTS_KMALLOC_ALIGN 16u

static uint8_t kmalloc_heap[BRIGHTS_KMALLOC_HEAP_SIZE];
static size_t kmalloc_next;

static size_t align_up(size_t v, size_t a)
{
  return (v + (a - 1u)) & ~(a - 1u);
}

void brights_kmalloc_init(void)
{
  kmalloc_next = 0;
}

void *brights_kmalloc(size_t size)
{
  if (size == 0) {
    return 0;
  }

  size_t start = align_up(kmalloc_next, BRIGHTS_KMALLOC_ALIGN);
  size_t end = start + size;
  if (end > BRIGHTS_KMALLOC_HEAP_SIZE) {
    return 0;
  }

  kmalloc_next = end;
  return &kmalloc_heap[start];
}

void brights_kfree(void *ptr)
{
  (void)ptr;
  // Early bump allocator: free is a no-op for now.
}
