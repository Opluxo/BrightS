#ifndef BRIGHTS_CACHE_H
#define BRIGHTS_CACHE_H

#include <stdint.h>
#include <stddef.h>

/*
 * BrightS Global Cache System
 *
 * Features:
 * - LRU eviction policy
 * - Configurable cache sizes
 * - Type-safe cache operations
 * - Statistics tracking
 * - Memory-efficient storage
 */

#define CACHE_MAX_NAME_LEN 32
#define CACHE_MAX_TYPES 16

/* Cache entry */
typedef struct cache_entry {
    uint64_t key;              /* Hash key */
    void *data;                /* Cached data */
    size_t size;               /* Data size */
    uint64_t access_time;      /* Last access timestamp */
    uint64_t create_time;      /* Creation timestamp */
    struct cache_entry *prev;  /* LRU list */
    struct cache_entry *next;  /* LRU list */
} cache_entry_t;

/* Cache statistics */
typedef struct cache_stats {
    uint64_t hits;             /* Cache hits */
    uint64_t misses;           /* Cache misses */
    uint64_t evictions;        /* Entries evicted */
    uint64_t inserts;          /* Entries inserted */
    uint64_t size_current;     /* Current cache size */
    uint64_t size_max;         /* Maximum cache size */
    uint64_t entries_current;  /* Current entry count */
    uint64_t entries_max;      /* Maximum entry count */
} cache_stats_t;

/* Cache configuration */
typedef struct cache_config {
    char name[CACHE_MAX_NAME_LEN];  /* Cache name */
    size_t max_size;               /* Maximum cache size in bytes */
    uint32_t max_entries;          /* Maximum number of entries */
    uint32_t ttl_seconds;          /* Time-to-live in seconds */
    void (*cleanup_func)(void *);  /* Cleanup function for entries */
} cache_config_t;

/* Cache handle */
typedef struct cache {
    cache_config_t config;
    cache_stats_t stats;

    /* Hash table */
    cache_entry_t **table;
    uint32_t table_size;

    /* LRU list */
    cache_entry_t *lru_head;
    cache_entry_t *lru_tail;

    /* Synchronization */
    uint32_t lock;  /* Simple spinlock */
} cache_t;

/* Global cache registry */
extern cache_t *global_caches[CACHE_MAX_TYPES];

/* Cache management API */
int cache_create(const cache_config_t *config, cache_t **cache_out);
int cache_destroy(cache_t *cache);
int cache_clear(cache_t *cache);

/* Cache operations */
int cache_put(cache_t *cache, uint64_t key, const void *data, size_t size);
int cache_get(cache_t *cache, uint64_t key, void **data_out, size_t *size_out);
int cache_remove(cache_t *cache, uint64_t key);
int cache_contains(cache_t *cache, uint64_t key);

/* Cache statistics */
const cache_stats_t *cache_get_stats(const cache_t *cache);
void cache_reset_stats(cache_t *cache);

/* Utility functions */
uint64_t cache_hash_string(const char *str);
uint64_t cache_hash_data(const void *data, size_t size);

/* Predefined cache types */
#define CACHE_TYPE_DNS      0  /* DNS resolution cache */
#define CACHE_TYPE_PATH     1  /* Path resolution cache */
#define CACHE_TYPE_INODE    2  /* Inode cache */
#define CACHE_TYPE_BUFFER   3  /* Buffer cache */
#define CACHE_TYPE_FONT     4  /* Font cache */
#define CACHE_TYPE_LOCALE   5  /* Locale data cache */

#endif /* BRIGHTS_CACHE_H */