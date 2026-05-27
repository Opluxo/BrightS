#include "kernel/stddef.h"
#include <stdint.h>
#include "kmalloc.h"

#define SLAB_CLASSES 10

static const size_t slab_sizes[SLAB_CLASSES] = {
  16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192
};

typedef struct slab_free {
  struct slab_free *next;
} slab_free_t;

typedef struct slab_page {
  struct slab_page *next;
  uint16_t class_idx;
  uint16_t free_count;
  uint16_t total_count;
  slab_free_t *free_list;
} slab_page_t;

typedef struct {
  slab_page_t *pages;
  size_t block_size;
} slab_class_t;

static slab_class_t slab_classes[SLAB_CLASSES];
static slab_page_t *recent_pages[SLAB_CLASSES];

static size_t kmalloc_total_used = 0;
static size_t kmalloc_slab_used = 0;
static size_t kmalloc_heap_used = 0;

#define KMALLOC_HEAP_SIZE (8u * 1024u * 1024u)
#define KMALLOC_ALIGN 16u
#define KMALLOC_MIN_BLOCK 32u

typedef struct kmalloc_block {
  size_t size;
  struct kmalloc_block *next;
} kmalloc_block_t;

static uint8_t kmalloc_heap[KMALLOC_HEAP_SIZE];
static kmalloc_block_t *free_list = 0;

extern void *brights_pmem_alloc_page(void);

static inline size_t align_up(size_t v, size_t a)
{
  return (v + (a - 1u)) & ~(a - 1u);
}

static inline void zero_mem(void *ptr, size_t len)
{
  uint64_t *p = (uint64_t *)ptr;
  size_t qwords = len / 8;
  for (size_t i = 0; i < qwords; ++i) p[i] = 0;
  len &= 7;
  if (len) {
    uint8_t *b = (uint8_t *)(p + qwords);
    for (size_t i = 0; i < len; ++i) b[i] = 0;
  }
}

/* Initialize a slab page */
static slab_page_t *slab_page_init(int class_idx)
{
  void *page = brights_pmem_alloc_page();
  if (!page) return 0;

  size_t block_size = slab_sizes[class_idx];
  uint32_t num_blocks = 4096 / block_size;
  /* Account for slab_page_t header embedded at start */
  size_t header_size = align_up(sizeof(slab_page_t), 16);
  num_blocks = (4096 - header_size) / block_size;
  if (num_blocks == 0) return 0;

  slab_page_t *sp = (slab_page_t *)page;
  sp->class_idx = class_idx;
  sp->free_count = num_blocks;
  sp->total_count = num_blocks;
  sp->next = 0;

  /* Build free list */
  uint8_t *data = (uint8_t *)page + header_size;
  slab_free_t *head = 0;
  for (uint32_t i = 0; i < num_blocks; ++i) {
    slab_free_t *block = (slab_free_t *)(data + i * block_size);
    block->next = head;
    head = block;
  }
  sp->free_list = head;

  return sp;
}

void brights_kmalloc_init(void)
{
  for (int i = 0; i < SLAB_CLASSES; ++i) {
    slab_classes[i].block_size = slab_sizes[i];
    slab_classes[i].pages = 0;
    recent_pages[i] = 0;
  }

  free_list = (kmalloc_block_t *)kmalloc_heap;
  free_list->size = KMALLOC_HEAP_SIZE;
  free_list->next = 0;

  kmalloc_total_used = 0;
  kmalloc_slab_used = 0;
  kmalloc_heap_used = 0;
}

/* Find slab class for given size (binary search with __builtin_expect) */
static inline int find_slab_class(size_t size)
{
  if (size == 0 || size > slab_sizes[SLAB_CLASSES - 1]) return -1;

  for (int i = 0; i < SLAB_CLASSES; ++i) {
    if (__builtin_expect(slab_sizes[i] >= size, i < 5)) {
      if (i == 0 || slab_sizes[i - 1] < size) return i;
    }
  }
  return -1;
}

void *brights_kmalloc(size_t size)
{
  if (size == 0) return 0;

  /* Try slab allocator for small allocations */
  int class_idx = find_slab_class(size);
  if (class_idx >= 0) {
    slab_class_t *sc = &slab_classes[class_idx];

    /* Try recently used page first for better cache locality */
    slab_page_t *recent = recent_pages[class_idx];
    if (recent && recent->free_count > 0 && recent->free_list) {
      slab_free_t *block = recent->free_list;
      recent->free_list = block->next;
      recent->free_count--;
      kmalloc_slab_used += sc->block_size;
      kmalloc_total_used += sc->block_size;
      zero_mem(block, sc->block_size);
      return (void *)block;
    }

    /* Try existing pages */
    for (slab_page_t *sp = sc->pages; sp; sp = sp->next) {
      if (sp->free_count > 0 && sp->free_list) {
        /* Update recent page cache */
        recent_pages[class_idx] = sp;

        slab_free_t *block = sp->free_list;
        sp->free_list = block->next;
        sp->free_count--;
        kmalloc_slab_used += sc->block_size;
        kmalloc_total_used += sc->block_size;
        zero_mem(block, sc->block_size);
        return (void *)block;
      }
    }

    /* Allocate new slab page */
    slab_page_t *new_page = slab_page_init(class_idx);
    if (new_page) {
      new_page->next = sc->pages;
      sc->pages = new_page;

      slab_free_t *block = new_page->free_list;
      new_page->free_list = block->next;
      new_page->free_count--;
      kmalloc_slab_used += sc->block_size;
      kmalloc_total_used += sc->block_size;
      zero_mem(block, sc->block_size);
      return (void *)block;
    }
    /* Fall through to heap if pmem exhausted */
  }

  /* Fallback heap allocator for large allocations (best-fit algorithm) */
  if (size > SIZE_MAX - sizeof(kmalloc_block_t)) {
    return NULL; /* Integer overflow protection */
  }
  size_t total_size = align_up(size + sizeof(kmalloc_block_t), KMALLOC_ALIGN);
  if (total_size < KMALLOC_MIN_BLOCK) total_size = KMALLOC_MIN_BLOCK;

  kmalloc_block_t *best_fit = 0;
  kmalloc_block_t *best_fit_prev = 0;
  kmalloc_block_t *curr = free_list;
  kmalloc_block_t *prev = 0;
  size_t best_fit_size = (size_t)-1;  /* Maximum size_t value */

  /* Find best-fit block */
  while (curr) {
    if (curr->size >= total_size && curr->size < best_fit_size) {
      best_fit = curr;
      best_fit_prev = prev;
      best_fit_size = curr->size;
    }
    prev = curr;
    curr = curr->next;
  }

  if (!best_fit) {
    return 0;  /* No suitable block found */
  }

  /* Split block if it's large enough */
  if (best_fit->size >= total_size + KMALLOC_MIN_BLOCK) {
    kmalloc_block_t *new_block = (kmalloc_block_t *)((uint8_t *)best_fit + total_size);
    new_block->size = best_fit->size - total_size;
    new_block->next = best_fit->next;
    best_fit->size = total_size;
    best_fit->next = new_block;
  }

  /* Remove from free list */
  if (best_fit_prev) best_fit_prev->next = best_fit->next;
  else free_list = best_fit->next;

  kmalloc_heap_used += best_fit->size;
  kmalloc_total_used += best_fit->size;
  return (void *)((uint8_t *)best_fit + sizeof(kmalloc_block_t));

  return 0;
}

void brights_kfree(void *ptr)
{
  if (!ptr) return;

  for (int i = 0; i < SLAB_CLASSES; ++i) {
    size_t block_size = slab_sizes[i];
    for (slab_page_t *sp = slab_classes[i].pages; sp; sp = sp->next) {
      uint8_t *page_start = (uint8_t *)sp;
      uint8_t *page_end = page_start + 4096;
      if ((uint8_t *)ptr >= page_start && (uint8_t *)ptr < page_end) {
        slab_free_t *block = (slab_free_t *)ptr;
        uint8_t *header_start = (uint8_t *)sp + align_up(sizeof(slab_page_t), 16);
        uint8_t *block_ptr = (uint8_t *)block;
        
        if (block_ptr < header_start || block_ptr >= (uint8_t *)sp + 4096) return;
        
        size_t block_offset = block_ptr - header_start;
        if (block_offset % block_size != 0) return;

        uint64_t *p64 = (uint64_t *)block_ptr;
        size_t qwords = block_size / 8;
        for (size_t j = 0; j < qwords; ++j) p64[j] = 0xCCCCCCCCCCCCCCCCULL;
        for (size_t j = qwords * 8; j < block_size; ++j) block_ptr[j] = 0xCC;

        block->next = sp->free_list;
        sp->free_list = block;
        sp->free_count++;
        kmalloc_slab_used -= block_size;
        kmalloc_total_used -= block_size;
        return;
      }
    }
  }

  kmalloc_block_t *block = (kmalloc_block_t *)((uint8_t *)ptr - sizeof(kmalloc_block_t));
  kmalloc_heap_used -= block->size;
  kmalloc_total_used -= block->size;

  kmalloc_block_t *prev = 0;
  kmalloc_block_t *curr = free_list;
  while (curr && curr < block) { prev = curr; curr = curr->next; }

  block->next = curr;
  if (prev) prev->next = block;
  else free_list = block;

  if (block->next && (uint8_t *)block + block->size == (uint8_t *)block->next) {
    block->size += block->next->size;
    block->next = block->next->next;
  }
  if (prev && (uint8_t *)prev + prev->size == (uint8_t *)block) {
    prev->size += block->size;
    prev->next = block->next;
  }
}

size_t brights_kmalloc_used(void)
{
  return kmalloc_total_used;
}

size_t brights_kmalloc_capacity(void)
{
  return KMALLOC_HEAP_SIZE + SLAB_CLASSES * 4096; /* Approximate */
}
