#ifndef BRIGHTS_RUST_FFI_H
#define BRIGHTS_RUST_FFI_H

#include <stdint.h>
#include <stddef.h>

/* ===== Slab allocator helpers ===== */
int32_t  rust_slab_find_class(size_t size);
int32_t  rust_slab_page_init(uint8_t *page, uint32_t class_idx);
uint8_t *rust_slab_alloc(struct slab_page *sp);
void     rust_slab_free(struct slab_page *sp, uint8_t *ptr);
int32_t  rust_slab_contains(struct slab_page *sp, uint8_t *ptr);

/* ===== Optimized memory operations ===== */
void *rust_memcpy(void *dst, const void *src, size_t n);
void *rust_memset(void *s, int c, size_t n);
int   rust_memcmp(const void *s1, const void *s2, size_t n);
void  rust_zero_mem(void *ptr, size_t len);

/* ===== Hash functions (FNV-1a / Murmur3) ===== */
uint32_t rust_hash_u64(uint64_t key, uint32_t table_size);
uint64_t rust_hash_string(const char *str);
uint64_t rust_hash_data(const void *data, size_t size);

/* ===== Ring buffer operations ===== */
int32_t rust_ringbuf_push(uint8_t *buf, uint64_t size, uint64_t *rd, uint64_t *wr, uint64_t *count, uint8_t byte);
int32_t rust_ringbuf_pop(uint8_t *buf, uint64_t size, uint64_t *rd, uint64_t *wr, uint64_t *count, uint8_t *byte);

#endif
