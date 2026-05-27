#include "cache.h"
#include "kmalloc.h"
#include "kernel_util.h"
#include "clock.h"

/* Global cache registry */
cache_t *global_caches[CACHE_MAX_TYPES] = {0};

/*
 * Initialize global cache system
 */
void brights_cache_init(void)
{
    /* Initialize predefined caches */

    /* DNS cache */
    cache_config_t dns_config = {
        .name = "dns",
        .max_size = 64 * 1024,  /* 64KB */
        .max_entries = 256,
        .ttl_seconds = 300,  /* 5 minutes */
        .cleanup_func = NULL
    };
    cache_create(&dns_config, &global_caches[CACHE_TYPE_DNS]);

    /* Path resolution cache (already exists in VFS, but add global one) */
    cache_config_t path_config = {
        .name = "path",
        .max_size = 128 * 1024,  /* 128KB */
        .max_entries = 512,
        .ttl_seconds = 0,  /* No expiration */
        .cleanup_func = NULL
    };
    cache_create(&path_config, &global_caches[CACHE_TYPE_PATH]);

    /* Inode cache */
    cache_config_t inode_config = {
        .name = "inode",
        .max_size = 256 * 1024,  /* 256KB */
        .max_entries = 1024,
        .ttl_seconds = 0,
        .cleanup_func = NULL
    };
    cache_create(&inode_config, &global_caches[CACHE_TYPE_INODE]);

    /* Buffer cache */
    cache_config_t buffer_config = {
        .name = "buffer",
        .max_size = 1024 * 1024,  /* 1MB */
        .max_entries = 2048,
        .ttl_seconds = 0,
        .cleanup_func = NULL
    };
    cache_create(&buffer_config, &global_caches[CACHE_TYPE_BUFFER]);
}

/*
 * Cleanup global cache system
 */
void brights_cache_cleanup(void)
{
    for (int i = 0; i < CACHE_MAX_TYPES; i++) {
        if (global_caches[i]) {
            cache_destroy(global_caches[i]);
            global_caches[i] = NULL;
        }
    }
}

/* Forward declarations */
static uint32_t cache_hash(uint64_t key, uint32_t table_size);
static void cache_evict_lru(cache_t *cache);
static cache_entry_t *cache_find_entry(cache_t *cache, uint64_t key);
static void cache_remove_entry(cache_t *cache, cache_entry_t *entry);
static void cache_add_to_lru(cache_t *cache, cache_entry_t *entry);
static void cache_remove_from_lru(cache_t *cache, cache_entry_t *entry);

/*
 * Create a new cache instance
 */
int cache_create(const cache_config_t *config, cache_t **cache_out)
{
    if (!config || !cache_out) return -1;
    if (config->max_size == 0 || config->max_entries == 0) return -1;

    cache_t *cache = brights_kmalloc(sizeof(cache_t));
    if (!cache) return -1;

    /* Copy configuration */
    kutil_memcpy(&cache->config, config, sizeof(cache_config_t));

    /* Initialize statistics */
    kutil_memset(&cache->stats, 0, sizeof(cache_stats_t));
    cache->stats.size_max = config->max_size;
    cache->stats.entries_max = config->max_entries;

    /* Initialize hash table */
    cache->table_size = config->max_entries / 4;  /* Load factor 0.25 */
    if (cache->table_size < 16) cache->table_size = 16;

    cache->table = brights_kmalloc(cache->table_size * sizeof(cache_entry_t *));
    if (!cache->table) {
        brights_kfree(cache);
        return -1;
    }
    kutil_memset(cache->table, 0, cache->table_size * sizeof(cache_entry_t *));

    /* Initialize LRU list */
    cache->lru_head = NULL;
    cache->lru_tail = NULL;
    cache->lock = 0;

    *cache_out = cache;
    return 0;
}

/*
 * Destroy a cache instance
 */
int cache_destroy(cache_t *cache)
{
    if (!cache) return -1;

    /* Clear all entries */
    cache_clear(cache);

    /* Free hash table */
    if (cache->table) {
        brights_kfree(cache->table);
    }

    /* Free cache structure */
    brights_kfree(cache);
    return 0;
}

/*
 * Clear all entries from cache
 */
int cache_clear(cache_t *cache)
{
    if (!cache) return -1;

    for (uint32_t i = 0; i < cache->table_size; i++) {
        cache_entry_t *entry = cache->table[i];
        while (entry) {
            cache_entry_t *next = entry->next;  /* Save next before freeing */

            /* Call cleanup function if provided */
            if (cache->config.cleanup_func) {
                cache->config.cleanup_func(entry->data);
            }

            /* Free entry data and structure */
            if (entry->data) brights_kfree(entry->data);
            brights_kfree(entry);

            entry = next;
        }
        cache->table[i] = NULL;
    }

    /* Reset LRU list */
    cache->lru_head = NULL;
    cache->lru_tail = NULL;

    /* Reset statistics */
    cache->stats.size_current = 0;
    cache->stats.entries_current = 0;
    cache->stats.evictions = 0;

    return 0;
}

/*
 * Hash function for cache keys
 */
static uint32_t cache_hash(uint64_t key, uint32_t table_size)
{
    /* Simple hash function - can be improved */
    return (uint32_t)((key * 2654435761UL) % table_size);
}

/*
 * Find entry by key
 */
static cache_entry_t *cache_find_entry(cache_t *cache, uint64_t key)
{
    uint32_t index = cache_hash(key, cache->table_size);
    cache_entry_t *entry = cache->table[index];

    while (entry) {
        if (entry->key == key) {
            return entry;
        }
        entry = entry->next;
    }

    return NULL;
}

/*
 * Remove entry from hash table and LRU list
 */
static void cache_remove_entry(cache_t *cache, cache_entry_t *entry)
{
    /* Remove from hash table */
    uint32_t index = cache_hash(entry->key, cache->table_size);
    cache_entry_t **slot = &cache->table[index];

    while (*slot) {
        if (*slot == entry) {
            *slot = entry->next;
            break;
        }
        slot = &(*slot)->next;
    }

    /* Remove from LRU list */
    cache_remove_from_lru(cache, entry);

    /* Update statistics */
    cache->stats.size_current -= entry->size;
    cache->stats.entries_current--;

    /* Free entry */
    if (entry->data && cache->config.cleanup_func) {
        cache->config.cleanup_func(entry->data);
    }
    if (entry->data) brights_kfree(entry->data);
    brights_kfree(entry);
}

/*
 * Add entry to LRU list (most recently used)
 */
static void cache_add_to_lru(cache_t *cache, cache_entry_t *entry)
{
    entry->prev = NULL;
    entry->next = cache->lru_head;

    if (cache->lru_head) {
        cache->lru_head->prev = entry;
    }
    cache->lru_head = entry;

    if (!cache->lru_tail) {
        cache->lru_tail = entry;
    }
}

/*
 * Remove entry from LRU list
 */
static void cache_remove_from_lru(cache_t *cache, cache_entry_t *entry)
{
    if (entry->prev) {
        entry->prev->next = entry->next;
    } else {
        cache->lru_head = entry->next;
    }

    if (entry->next) {
        entry->next->prev = entry->prev;
    } else {
        cache->lru_tail = entry->prev;
    }

    entry->prev = NULL;
    entry->next = NULL;
}

/*
 * Evict least recently used entry
 */
static void cache_evict_lru(cache_t *cache)
{
    if (!cache->lru_tail) return;

    cache_entry_t *victim = cache->lru_tail;
    cache_remove_entry(cache, victim);
    cache->stats.evictions++;
}

/*
 * Put data into cache
 */
int cache_put(cache_t *cache, uint64_t key, const void *data, size_t size)
{
    if (!cache || !data || size == 0) return -1;

    /* Check if entry already exists */
    cache_entry_t *existing = cache_find_entry(cache, key);
    if (existing) {
        /* Update existing entry */
        cache_remove_from_lru(cache, existing);

        /* Reallocate data if size changed */
        if (existing->size != size) {
            void *new_data = brights_kmalloc(size);
            if (!new_data) return -1;

            if (existing->data) brights_kfree(existing->data);
            existing->data = new_data;
            cache->stats.size_current = cache->stats.size_current - existing->size + size;
            existing->size = size;
        }

        kutil_memcpy(existing->data, data, size);
        existing->access_time = brights_clock_ms();
        cache_add_to_lru(cache, existing);

        return 0;
    }

    /* Check size limits */
    if (cache->stats.entries_current >= cache->stats.entries_max ||
        cache->stats.size_current + size > cache->stats.size_max) {
        cache_evict_lru(cache);
    }

    /* Still can't fit? */
    if (cache->stats.entries_current >= cache->stats.entries_max ||
        cache->stats.size_current + size > cache->stats.size_max) {
        return -1;  /* Cache full */
    }

    /* Create new entry */
    cache_entry_t *entry = brights_kmalloc(sizeof(cache_entry_t));
    if (!entry) return -1;

    entry->data = brights_kmalloc(size);
    if (!entry->data) {
        brights_kfree(entry);
        return -1;
    }

    entry->key = key;
    entry->size = size;
    entry->create_time = brights_clock_ms();
    entry->access_time = entry->create_time;
    kutil_memcpy(entry->data, data, size);

    /* Add to hash table */
    uint32_t index = cache_hash(key, cache->table_size);
    entry->next = cache->table[index];
    cache->table[index] = entry;

    /* Add to LRU list */
    cache_add_to_lru(cache, entry);

    /* Update statistics */
    cache->stats.inserts++;
    cache->stats.entries_current++;
    cache->stats.size_current += size;

    return 0;
}

/*
 * Get data from cache
 */
int cache_get(cache_t *cache, uint64_t key, void **data_out, size_t *size_out)
{
    if (!cache || !data_out || !size_out) return -1;

    cache_entry_t *entry = cache_find_entry(cache, key);
    if (!entry) {
        cache->stats.misses++;
        return -1;  /* Not found */
    }

    /* Check TTL */
    if (cache->config.ttl_seconds > 0) {
        uint64_t current_time = brights_clock_ms();
        uint64_t age_seconds = (current_time - entry->create_time) / 1000;
        if (age_seconds >= cache->config.ttl_seconds) {
            cache_remove_entry(cache, entry);
            cache->stats.misses++;
            return -1;  /* Expired */
        }
    }

    /* Update access time and move to front of LRU */
    entry->access_time = brights_clock_ms();
    cache_remove_from_lru(cache, entry);
    cache_add_to_lru(cache, entry);

    *data_out = entry->data;
    *size_out = entry->size;
    cache->stats.hits++;

    return 0;
}

/*
 * Remove entry from cache
 */
int cache_remove(cache_t *cache, uint64_t key)
{
    if (!cache) return -1;

    cache_entry_t *entry = cache_find_entry(cache, key);
    if (!entry) return -1;

    cache_remove_entry(cache, entry);
    return 0;
}

/*
 * Check if key exists in cache
 */
int cache_contains(cache_t *cache, uint64_t key)
{
    if (!cache) return 0;
    return cache_find_entry(cache, key) != NULL;
}

/*
 * Get cache statistics
 */
const cache_stats_t *cache_get_stats(const cache_t *cache)
{
    return cache ? &cache->stats : NULL;
}

/*
 * Reset cache statistics
 */
void cache_reset_stats(cache_t *cache)
{
    if (!cache) return;

    cache->stats.hits = 0;
    cache->stats.misses = 0;
    cache->stats.evictions = 0;
    cache->stats.inserts = 0;
    /* Keep size and entry counts */
}

/*
 * Hash utility functions
 */
uint64_t cache_hash_string(const char *str)
{
    if (!str) return 0;

    uint64_t hash = 0;
    while (*str) {
        hash = hash * 31 + (uint64_t)*str;
        str++;
    }
    return hash;
}

uint64_t cache_hash_data(const void *data, size_t size)
{
    if (!data || size == 0) return 0;

    const uint8_t *bytes = (const uint8_t *)data;
    uint64_t hash = 0;

    for (size_t i = 0; i < size; i++) {
        hash = hash * 31 + (uint64_t)bytes[i];
    }

    return hash;
}