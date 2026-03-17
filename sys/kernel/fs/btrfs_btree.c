#include "btrfs_btree.h"
#include "btrfs.h"
#include "../dev/block.h"
#include <stdint.h>

// Minimal leaf insertion: no split, no balancing.
// TODO: add split/merge once a block allocator is available.

typedef struct {
  uint8_t csum[32];
  uint8_t fsid[16];
  uint64_t bytenr;
  uint64_t flags;
  uint8_t chunk_tree_uuid[16];
  uint64_t generation;
  uint64_t owner;
  uint32_t nritems;
  uint8_t level;
} __attribute__((packed)) brights_btrfs_header_t;

typedef struct {
  uint64_t objectid;
  uint8_t type;
  uint64_t offset;
} __attribute__((packed)) brights_btrfs_key_t;

typedef struct {
  brights_btrfs_key_t key;
  uint32_t offset;
  uint32_t size;
} __attribute__((packed)) brights_btrfs_item_t;

extern uint32_t btrfs_nodesize;
extern int brights_btrfs_raw_read(uint64_t bytenr, void *dst, uint64_t size);
extern int brights_btrfs_raw_write(uint64_t bytenr, const void *src, uint64_t size);

static int key_cmp(const brights_btrfs_key_t *a, const brights_btrfs_key_t *b)
{
  if (a->objectid != b->objectid) {
    return (a->objectid < b->objectid) ? -1 : 1;
  }
  if (a->type != b->type) {
    return (a->type < b->type) ? -1 : 1;
  }
  if (a->offset != b->offset) {
    return (a->offset < b->offset) ? -1 : 1;
  }
  return 0;
}

int brights_btrfs_btree_insert_leaf(uint64_t bytenr, const void *key, const void *item, uint32_t item_size)
{
  if (btrfs_nodesize == 0 || item_size == 0) {
    return -1;
  }
  uint8_t buf[16384];
  if (btrfs_nodesize > sizeof(buf)) {
    return -1;
  }
  if (brights_btrfs_raw_read(bytenr, buf, btrfs_nodesize) != 0) {
    return -1;
  }

  brights_btrfs_header_t *hdr = (brights_btrfs_header_t *)buf;
  if (hdr->level != 0) {
    return -1;
  }

  brights_btrfs_item_t *items = (brights_btrfs_item_t *)(buf + sizeof(brights_btrfs_header_t));
  uint32_t nritems = hdr->nritems;
  uint32_t data_start = btrfs_nodesize;
  for (uint32_t i = 0; i < nritems; ++i) {
    if (items[i].offset < data_start) {
      data_start = items[i].offset;
    }
  }

  uint32_t new_item_end = data_start;
  uint32_t new_item_off = new_item_end - item_size;
  uint32_t items_end = sizeof(brights_btrfs_header_t) + (nritems + 1) * sizeof(brights_btrfs_item_t);
  if (new_item_off < items_end) {
    return -2; // no space, split needed
  }

  // Find insert position to keep items sorted by key.
  uint32_t pos = 0;
  const brights_btrfs_key_t *new_key = (const brights_btrfs_key_t *)key;
  for (; pos < nritems; ++pos) {
    if (key_cmp(new_key, &items[pos].key) < 0) {
      break;
    }
  }
  // Shift item array to make room.
  for (uint32_t i = nritems; i > pos; --i) {
    items[i] = items[i - 1];
  }
  brights_btrfs_item_t *it = &items[pos];
  it->key = *new_key;
  it->offset = new_item_off;
  it->size = item_size;

  // Copy item payload
  uint8_t *dst = buf + new_item_off;
  const uint8_t *src = (const uint8_t *)item;
  for (uint32_t i = 0; i < item_size; ++i) {
    dst[i] = src[i];
  }

  hdr->nritems = nritems + 1;

  return brights_btrfs_raw_write(bytenr, buf, btrfs_nodesize);
}
