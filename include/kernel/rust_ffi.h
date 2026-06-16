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

/* ===== Fixed-size hash map (Robin Hood open addressing) ===== */
typedef struct {
    uint64_t key;
    uint64_t value;
    uint32_t dist;
} rust_hashmap_entry_t;

typedef struct {
    rust_hashmap_entry_t *entries;
    uint32_t capacity;
    uint32_t size;
    uint32_t mask;
} rust_hashmap_state_t;

rust_hashmap_state_t rust_hashmap_init(rust_hashmap_entry_t *entries, uint32_t capacity);
int32_t  rust_hashmap_get(rust_hashmap_state_t *state, uint64_t key);
int32_t  rust_hashmap_insert(rust_hashmap_state_t *state, uint64_t key, uint64_t value);
int32_t  rust_hashmap_remove(rust_hashmap_state_t *state, uint64_t key);
uint32_t rust_hashmap_size(const rust_hashmap_state_t *state);

/* ===== LRU cache (O(1) hash + doubly-linked list) ===== */
typedef struct {
    uint64_t key;
    uint64_t value;
    int32_t  prev;
    int32_t  next;
    int32_t  hash_next;
} rust_lru_node_t;

typedef struct {
    rust_lru_node_t *nodes;
    int32_t         *hash_table;
    uint32_t        hash_size;
    uint32_t        hash_mask;
    uint32_t        capacity;
    uint32_t        size;
    int32_t         head;
    int32_t         tail;
} rust_lru_cache_t;

rust_lru_cache_t rust_lru_init(rust_lru_node_t *nodes, int32_t *hash_table, uint32_t hash_size, uint32_t capacity);
int32_t  rust_lru_get(rust_lru_cache_t *state, uint64_t key);
int32_t  rust_lru_put(rust_lru_cache_t *state, uint64_t key, uint64_t value);
int32_t  rust_lru_remove(rust_lru_cache_t *state, uint64_t key);
uint32_t rust_lru_size(const rust_lru_cache_t *state);

/* ===== Process table RAII cleanup ===== */
typedef struct {
    uint8_t in_use;
    uint8_t fd_type;  /* 0=none, 1=file, 2=pipe, 3=socket, 4=tty */
} rust_proc_fd_info_t;

int32_t rust_proc_cleanup_fds(const rust_proc_fd_info_t *fds, uint32_t max_fds, int32_t (*close_fn)(uint32_t));
int32_t rust_proc_owns_page(const uint8_t *ptr, uint64_t proc_start, uint64_t proc_size);
void    rust_proc_reset_buffer(uint8_t *buf, uint64_t size, uint64_t *wr_ptr);

/* ===== Ring buffer operations ===== */
int32_t rust_ringbuf_push(uint8_t *buf, uint64_t size, uint64_t *rd, uint64_t *wr, uint64_t *count, uint8_t byte);
int32_t rust_ringbuf_pop(uint8_t *buf, uint64_t size, uint64_t *rd, uint64_t *wr, uint64_t *count, uint8_t *byte);

/* ===== Network packet validation (safe parsing) ===== */
int32_t  rust_ip_validate(const uint8_t *frame, uint32_t frame_len, uint32_t *ip_hdr_len, const uint8_t **payload, uint32_t *payload_len);
int32_t  rust_tcp_validate(const uint8_t *tcp_data, uint32_t tcp_len, uint32_t *data_off, const uint8_t **payload, uint32_t *payload_len);
uint16_t rust_ip_checksum(const uint8_t *data, uint32_t len);

#endif
