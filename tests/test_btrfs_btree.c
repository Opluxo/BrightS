#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "../sys/kernel/fs/btrfs_btree.h"

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
} __attribute__((packed)) test_btrfs_header_t;

typedef struct {
  uint64_t objectid;
  uint8_t type;
  uint64_t offset;
} __attribute__((packed)) test_btrfs_key_t;

typedef struct {
  test_btrfs_key_t key;
  uint32_t offset;
  uint32_t size;
} __attribute__((packed)) test_btrfs_item_t;

uint32_t btrfs_nodesize = 4096;

static uint8_t fake_node[4096];

int brights_btrfs_raw_read(uint64_t bytenr, void *dst, uint64_t size)
{
  (void)bytenr;
  memcpy(dst, fake_node, size);
  return 0;
}

int brights_btrfs_raw_write(uint64_t bytenr, const void *src, uint64_t size)
{
  (void)bytenr;
  memcpy(fake_node, src, size);
  return 0;
}

static void reset_leaf(void)
{
  memset(fake_node, 0, sizeof(fake_node));
  ((test_btrfs_header_t *)fake_node)->level = 0;
}

static test_btrfs_item_t *items(void)
{
  return (test_btrfs_item_t *)(fake_node + sizeof(test_btrfs_header_t));
}

static void test_insert_into_empty_leaf(void)
{
  reset_leaf();

  test_btrfs_key_t key = {.objectid = 5, .type = 1, .offset = 9};
  const char payload[] = "abc";

  int rc = brights_btrfs_btree_insert_leaf(0, &key, payload, sizeof(payload));
  assert(rc == 0);

  test_btrfs_header_t *hdr = (test_btrfs_header_t *)fake_node;
  assert(hdr->nritems == 1);
  assert(items()[0].key.objectid == 5);
  assert(items()[0].key.type == 1);
  assert(items()[0].key.offset == 9);
  assert(items()[0].size == sizeof(payload));
  assert(memcmp(fake_node + items()[0].offset, payload, sizeof(payload)) == 0);
}

static void test_insert_keeps_sorted_order(void)
{
  reset_leaf();

  test_btrfs_key_t key_b = {.objectid = 20, .type = 1, .offset = 0};
  test_btrfs_key_t key_a = {.objectid = 10, .type = 1, .offset = 0};
  const char payload_b[] = "bbb";
  const char payload_a[] = "aaa";

  assert(brights_btrfs_btree_insert_leaf(0, &key_b, payload_b, sizeof(payload_b)) == 0);
  assert(brights_btrfs_btree_insert_leaf(0, &key_a, payload_a, sizeof(payload_a)) == 0);

  test_btrfs_header_t *hdr = (test_btrfs_header_t *)fake_node;
  assert(hdr->nritems == 2);
  assert(items()[0].key.objectid == 10);
  assert(items()[1].key.objectid == 20);
  assert(memcmp(fake_node + items()[0].offset, payload_a, sizeof(payload_a)) == 0);
  assert(memcmp(fake_node + items()[1].offset, payload_b, sizeof(payload_b)) == 0);
}

static void test_insert_reports_full_leaf(void)
{
  reset_leaf();

  test_btrfs_header_t *hdr = (test_btrfs_header_t *)fake_node;
  hdr->nritems = 1;
  items()[0].key.objectid = 1;
  items()[0].key.type = 1;
  items()[0].key.offset = 0;
  items()[0].offset = sizeof(test_btrfs_header_t) + sizeof(test_btrfs_item_t) + 4;
  items()[0].size = 4;

  test_btrfs_key_t key = {.objectid = 2, .type = 1, .offset = 0};
  const char payload[] = "1234";

  int rc = brights_btrfs_btree_insert_leaf(0, &key, payload, sizeof(payload));
  assert(rc == -2);
}

int main(void)
{
  test_insert_into_empty_leaf();
  test_insert_keeps_sorted_order();
  test_insert_reports_full_leaf();
  return 0;
}
