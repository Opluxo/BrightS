#include <stddef.h>
#include <stdint.h>

// Optimized for i5 1135G7 + 8GB RAM: 8MB kernel heap
#define BRIGHTS_KMALLOC_HEAP_SIZE (8u * 1024u * 1024u)
#define BRIGHTS_KMALLOC_ALIGN 16u
#define BRIGHTS_KMALLOC_MIN_BLOCK 32u

typedef struct kmalloc_block {
  size_t size;
  struct kmalloc_block *next;
} kmalloc_block_t;

static uint8_t kmalloc_heap[BRIGHTS_KMALLOC_HEAP_SIZE];
static kmalloc_block_t *free_list = 0;
static size_t kmalloc_used = 0;

static size_t align_up(size_t v, size_t a)
{
  return (v + (a - 1u)) & ~(a - 1u);
}

void brights_kmalloc_init(void)
{
  free_list = (kmalloc_block_t *)kmalloc_heap;
  free_list->size = BRIGHTS_KMALLOC_HEAP_SIZE;
  free_list->next = 0;
  kmalloc_used = 0;
}

void *brights_kmalloc(size_t size)
{
  if (size == 0) {
    return 0;
  }

  size_t total_size = align_up(size + sizeof(kmalloc_block_t), BRIGHTS_KMALLOC_ALIGN);
  if (total_size < BRIGHTS_KMALLOC_MIN_BLOCK) {
    total_size = BRIGHTS_KMALLOC_MIN_BLOCK;
  }

  kmalloc_block_t *prev = 0;
  kmalloc_block_t *curr = free_list;

  while (curr) {
    if (curr->size >= total_size) {
      // Found a suitable block
      if (curr->size >= total_size + BRIGHTS_KMALLOC_MIN_BLOCK) {
        // Split the block
        kmalloc_block_t *new_block = (kmalloc_block_t *)((uint8_t *)curr + total_size);
        new_block->size = curr->size - total_size;
        new_block->next = curr->next;
        curr->size = total_size;
        curr->next = new_block;
      }

      // Remove from free list
      if (prev) {
        prev->next = curr->next;
      } else {
        free_list = curr->next;
      }

      kmalloc_used += curr->size;
      return (void *)((uint8_t *)curr + sizeof(kmalloc_block_t));
    }
    prev = curr;
    curr = curr->next;
  }

  return 0; // Out of memory
}

void brights_kfree(void *ptr)
{
  if (!ptr) {
    return;
  }

  kmalloc_block_t *block = (kmalloc_block_t *)((uint8_t *)ptr - sizeof(kmalloc_block_t));
  kmalloc_used -= block->size;

  // Insert into free list in address order for coalescing
  kmalloc_block_t *prev = 0;
  kmalloc_block_t *curr = free_list;

  while (curr && curr < block) {
    prev = curr;
    curr = curr->next;
  }

  // Insert block
  block->next = curr;
  if (prev) {
    prev->next = block;
  } else {
    free_list = block;
  }

  // Coalesce with next block if adjacent
  if (block->next && (uint8_t *)block + block->size == (uint8_t *)block->next) {
    block->size += block->next->size;
    block->next = block->next->next;
  }

  // Coalesce with previous block if adjacent
  if (prev && (uint8_t *)prev + prev->size == (uint8_t *)block) {
    prev->size += block->size;
    prev->next = block->next;
  }
}

size_t brights_kmalloc_used(void)
{
  return kmalloc_used;
}

size_t brights_kmalloc_capacity(void)
{
  return BRIGHTS_KMALLOC_HEAP_SIZE;
}
