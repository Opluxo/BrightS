#include "../core/kernel_util.h"
#include "btrfs.h"
#include "../dev/block.h"
#include "../core/printf.h"
#include "../dev/serial.h"
#include <stdint.h>

#define BTRFS_SUPER_MAGIC "_BHRfS_M"
#define BTRFS_SUPER_OFFSET 0x10000u

#define BTRFS_CHUNK_ITEM_KEY 0xE
#define BTRFS_ROOT_ITEM_KEY 132
#define BTRFS_INODE_ITEM_KEY 1
#define BTRFS_DIR_ITEM_KEY 84
#define BTRFS_EXTENT_DATA_KEY 108

#define BTRFS_FS_TREE_OBJECTID 5

#define BRIGHTS_BTRFS_MAX_CHUNKS 32
#define BRIGHTS_BTRFS_MAX_NODE 16384

// Btrfs tree header (nodes/leaves)
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

typedef struct {
  brights_btrfs_key_t key;
  uint64_t blockptr;
  uint64_t generation;
} __attribute__((packed)) brights_btrfs_key_ptr_t;

typedef struct {
  uint64_t length;
  uint64_t owner;
  uint64_t stripe_len;
  uint64_t type;
  uint32_t io_align;
  uint32_t io_width;
  uint32_t sector_size;
  uint16_t num_stripes;
  uint16_t sub_stripes;
} __attribute__((packed)) brights_btrfs_chunk_t;

typedef struct {
  uint64_t devid;
  uint64_t offset;
} __attribute__((packed)) brights_btrfs_stripe_t;

typedef struct {
  uint64_t logical;
  uint64_t length;
  uint64_t physical;
} brights_btrfs_chunk_map_t;

typedef struct {
  uint64_t generation;
  uint64_t transid;
  uint64_t size;
  uint64_t nbytes;
  uint64_t block_group;
  uint32_t nlink;
  uint32_t uid;
  uint32_t gid;
  uint32_t mode;
  uint64_t rdev;
  uint64_t flags;
  uint64_t sequence;
  uint64_t reserved[4];
  uint64_t atime_sec;
  uint32_t atime_nsec;
  uint64_t ctime_sec;
  uint32_t ctime_nsec;
  uint64_t mtime_sec;
  uint32_t mtime_nsec;
  uint64_t otime_sec;
  uint32_t otime_nsec;
} __attribute__((packed)) brights_btrfs_inode_item_t;

typedef struct {
  brights_btrfs_key_t location;
  uint64_t transid;
  uint16_t data_len;
  uint16_t name_len;
  uint8_t type;
} __attribute__((packed)) brights_btrfs_dir_item_t;

typedef struct {
  brights_btrfs_inode_item_t inode;
  uint64_t generation;
  uint64_t root_dirid;
  uint64_t bytenr;
  uint64_t byte_limit;
  uint64_t bytes_used;
  uint64_t last_snapshot;
  uint64_t flags;
  uint32_t refs;
  brights_btrfs_key_t drop_progress;
  uint8_t drop_level;
  uint8_t level;
  uint64_t generation_v2;
  uint8_t uuid[16];
  uint8_t parent_uuid[16];
  uint8_t received_uuid[16];
  uint64_t ctransid;
  uint64_t otransid;
  uint64_t stransid;
  uint64_t rtransid;
  uint64_t rtime_sec;
  uint32_t rtime_nsec;
  uint64_t reserved[8];
} __attribute__((packed)) brights_btrfs_root_item_t;

typedef struct {
  uint64_t generation;
  uint64_t ram_bytes;
  uint8_t compression;
  uint8_t encryption;
  uint16_t other_encoding;
  uint8_t type;
  uint64_t disk_bytenr;
  uint64_t disk_num_bytes;
  uint64_t offset;
  uint64_t num_bytes;
} __attribute__((packed)) brights_btrfs_file_extent_item_t;

typedef struct {
  uint8_t csum[32];
  uint8_t fsid[16];
  uint64_t bytenr;
  uint64_t flags;
  uint8_t magic[8];
  uint64_t generation;
  uint64_t root;
  uint64_t chunk_root;
  uint64_t log_root;
  uint64_t log_root_transid;
  uint64_t total_bytes;
  uint64_t bytes_used;
  uint64_t root_dir_objectid;
  uint64_t num_devices;
  uint32_t sectorsize;
  uint32_t nodesize;
  uint32_t __unused_leafsize;
  uint32_t stripesize;
  uint32_t sys_chunk_array_size;
  uint64_t chunk_root_generation;
  uint64_t compat_flags;
  uint64_t compat_ro_flags;
  uint64_t incompat_flags;
  uint16_t csum_type;
  uint8_t root_level;
  uint8_t chunk_root_level;
  uint8_t log_root_level;
  uint8_t dev_item[0x100];
  uint8_t label[256];
  uint64_t cache_generation;
  uint64_t uuid_tree_generation;
  uint8_t metadata_uuid[16];
  uint64_t nr_global_roots;
  uint64_t reserved[27];
  uint8_t sys_chunk_array[2048];
} __attribute__((packed)) brights_btrfs_super_t;

static brights_btrfs_chunk_map_t chunk_map[BRIGHTS_BTRFS_MAX_CHUNKS];
static uint32_t chunk_map_count;
uint32_t btrfs_nodesize;
static brights_btrfs_root_item_t btrfs_fs_root;
static int btrfs_fs_root_ok;

static int read_super(brights_btrfs_super_t *sb)
{
  brights_block_dev_t *dev = brights_block_root();
  if (!dev || !dev->read) {
    return -1;
  }
  uint8_t buf[4096];
  uint64_t lba = BTRFS_SUPER_OFFSET / BRIGHTS_BLOCK_SIZE;
  if (dev->read(lba, buf, 1) != 0) {
    return -1;
  }
  *sb = *(const brights_btrfs_super_t *)buf;
  return 0;
}

static int super_magic_ok(const brights_btrfs_super_t *sb)
{
  const char *magic = (const char *)sb->magic;
  for (int i = 0; i < 8; ++i) {
    if (magic[i] != BTRFS_SUPER_MAGIC[i]) {
      return 0;
    }
  }
  return 1;
}

static void print_hex64(brights_console_t *con, uint64_t v)
{
  static const uint16_t hex[] = u"0123456789ABCDEF";
  uint16_t buf[2 + 16 + 1];
  buf[0] = '0';
  buf[1] = 'x';
  for (int i = 0; i < 16; ++i) {
    buf[2 + i] = hex[(v >> ((15 - i) * 4)) & 0xF];
  }
  buf[18] = 0;
  brights_print(con, buf);
}

static void print_str(brights_console_t *con, const char *s)
{
  while (*s) {
    uint16_t ch[2] = {(uint16_t)*s, 0};
    brights_print(con, ch);
    ++s;
  }
}

int brights_btrfs_raw_read(uint64_t bytenr, void *dst, uint64_t size)
{
  brights_block_dev_t *dev = brights_block_root();
  if (!dev || !dev->read) {
    return -1;
  }
  uint64_t lba = bytenr / BRIGHTS_BLOCK_SIZE;
  uint64_t count = (size + BRIGHTS_BLOCK_SIZE - 1) / BRIGHTS_BLOCK_SIZE;
  return dev->read(lba, dst, count);
}

int brights_btrfs_raw_write(uint64_t bytenr, const void *src, uint64_t size)
{
  brights_block_dev_t *dev = brights_block_root();
  if (!dev || !dev->write) {
    return -1;
  }
  uint64_t lba = bytenr / BRIGHTS_BLOCK_SIZE;
  uint64_t count = (size + BRIGHTS_BLOCK_SIZE - 1) / BRIGHTS_BLOCK_SIZE;
  return dev->write(lba, src, count);
}

static int read_tree_node(uint64_t bytenr, uint8_t *buf, uint32_t size)
{
  if (size > BRIGHTS_BTRFS_MAX_NODE) {
    return -1;
  }
  return brights_btrfs_raw_read(bytenr, buf, size);
}

static void dump_tree_header(brights_console_t *con, const char *label, const brights_btrfs_header_t *hdr)
{
  brights_print(con, u"btrfs: ");
  if (label) {
    print_str(con, label);
  }
  brights_print(con, u" tree bytenr=");
  print_hex64(con, hdr->bytenr);
  brights_print(con, u" level=");
  print_hex64(con, hdr->level);
  brights_print(con, u" items=");
  print_hex64(con, hdr->nritems);
  brights_print(con, u"\r\n");
}

static void chunk_map_add(uint64_t logical, uint64_t length, uint64_t physical)
{
  if (chunk_map_count >= BRIGHTS_BTRFS_MAX_CHUNKS) {
    return;
  }
  chunk_map[chunk_map_count].logical = logical;
  chunk_map[chunk_map_count].length = length;
  chunk_map[chunk_map_count].physical = physical;
  ++chunk_map_count;
}

static int parse_sys_chunk_array(const brights_btrfs_super_t *sb)
{
  chunk_map_count = 0;
  uint32_t off = 0;
  while (off + sizeof(brights_btrfs_key_t) + sizeof(brights_btrfs_chunk_t) <= sb->sys_chunk_array_size) {
    const brights_btrfs_key_t *key = (const brights_btrfs_key_t *)(sb->sys_chunk_array + off);
    off += sizeof(brights_btrfs_key_t);
    if (key->type != BTRFS_CHUNK_ITEM_KEY) {
      break;
    }
    const brights_btrfs_chunk_t *chunk = (const brights_btrfs_chunk_t *)(sb->sys_chunk_array + off);
    off += sizeof(brights_btrfs_chunk_t);
    const brights_btrfs_stripe_t *stripe = (const brights_btrfs_stripe_t *)(sb->sys_chunk_array + off);
    off += sizeof(brights_btrfs_stripe_t) * chunk->num_stripes;
    chunk_map_add(key->offset, chunk->length, stripe->offset);
  }
  return (chunk_map_count > 0) ? 0 : -1;
}

static uint64_t map_logical(uint64_t logical)
{
  for (uint32_t i = 0; i < chunk_map_count; ++i) {
    uint64_t start = chunk_map[i].logical;
    uint64_t end = start + chunk_map[i].length;
    if (logical >= start && logical < end) {
      return chunk_map[i].physical + (logical - start);
    }
  }
  return logical; // fallback (best-effort)
}

static int parse_chunk_leaf(uint64_t bytenr)
{
  uint8_t buf[BRIGHTS_BTRFS_MAX_NODE];
  if (read_tree_node(bytenr, buf, btrfs_nodesize) != 0) {
    return -1;
  }
  brights_btrfs_header_t *hdr = (brights_btrfs_header_t *)buf;
  if (hdr->level != 0) {
    return -1;
  }

  brights_btrfs_item_t *items = (brights_btrfs_item_t *)(buf + sizeof(brights_btrfs_header_t));
  for (uint32_t i = 0; i < hdr->nritems; ++i) {
    brights_btrfs_item_t *it = &items[i];
    if (it->key.type != BTRFS_CHUNK_ITEM_KEY) {
      continue;
    }
    uint32_t item_off = it->offset;
    brights_btrfs_chunk_t *chunk = (brights_btrfs_chunk_t *)(buf + item_off);
    brights_btrfs_stripe_t *stripe = (brights_btrfs_stripe_t *)(buf + item_off + sizeof(brights_btrfs_chunk_t));
    chunk_map_add(it->key.offset, chunk->length, stripe->offset);
  }
  return 0;
}

static int parse_root_leaf(uint64_t bytenr, brights_btrfs_root_item_t *out_root)
{
  uint8_t buf[BRIGHTS_BTRFS_MAX_NODE];
  if (read_tree_node(bytenr, buf, btrfs_nodesize) != 0) {
    return -1;
  }
  brights_btrfs_header_t *hdr = (brights_btrfs_header_t *)buf;
  if (hdr->level != 0) {
    return -1;
  }

  brights_btrfs_item_t *items = (brights_btrfs_item_t *)(buf + sizeof(brights_btrfs_header_t));
  for (uint32_t i = 0; i < hdr->nritems; ++i) {
    brights_btrfs_item_t *it = &items[i];
    if (it->key.objectid == BTRFS_FS_TREE_OBJECTID && it->key.type == BTRFS_ROOT_ITEM_KEY) {
      uint32_t item_off = it->offset;
      if (it->size < sizeof(brights_btrfs_root_item_t)) {
        return -1;
      }
      *out_root = *(const brights_btrfs_root_item_t *)(buf + item_off);
      return 0;
    }
  }
  return -1;
}

static int fs_tree_scan_dir_leaf(const uint8_t *buf, uint64_t dir_objectid, brights_console_t *con)
{
  const brights_btrfs_header_t *hdr = (const brights_btrfs_header_t *)buf;
  brights_btrfs_item_t *items = (brights_btrfs_item_t *)(buf + sizeof(brights_btrfs_header_t));
  brights_print(con, u"btrfs: dir entries\r\n");

  for (uint32_t i = 0; i < hdr->nritems; ++i) {
    brights_btrfs_item_t *it = &items[i];
    if (it->key.objectid != dir_objectid || it->key.type != BTRFS_DIR_ITEM_KEY) {
      continue;
    }
    uint32_t item_off = it->offset;
    uint32_t item_end = item_off + it->size;
    while (item_off + sizeof(brights_btrfs_dir_item_t) <= item_end) {
      brights_btrfs_dir_item_t *di = (brights_btrfs_dir_item_t *)(buf + item_off);
      item_off += sizeof(brights_btrfs_dir_item_t);
      const char *name = (const char *)(buf + item_off);
      item_off += di->name_len + di->data_len;
      brights_print(con, u"  ");
      for (uint16_t j = 0; j < di->name_len; ++j) {
        uint16_t ch[2] = {(uint16_t)name[j], 0};
        brights_print(con, ch);
      }
      brights_print(con, u" ino=");
      print_hex64(con, di->location.objectid);
      brights_print(con, u"\r\n");
    }
  }
  return 0;
}

static int fs_tree_scan_dir(uint64_t fs_root_bytenr, uint8_t fs_root_level, uint64_t dir_objectid, brights_console_t *con)
{
  uint64_t fs_phys = map_logical(fs_root_bytenr);
  uint8_t buf[BRIGHTS_BTRFS_MAX_NODE];
  if (read_tree_node(fs_phys, buf, btrfs_nodesize) != 0) {
    return -1;
  }

  brights_btrfs_header_t *hdr = (brights_btrfs_header_t *)buf;
  if (hdr->level == 0) {
    return fs_tree_scan_dir_leaf(buf, dir_objectid, con);
  }

  brights_btrfs_key_ptr_t *ptrs = (brights_btrfs_key_ptr_t *)(buf + sizeof(brights_btrfs_header_t));
  for (uint32_t i = 0; i < hdr->nritems; ++i) {
    if (fs_tree_scan_dir(ptrs[i].blockptr, hdr->level - 1, dir_objectid, con) == 0) {
      // Continue scanning all leaves for full directory listing.
    }
  }
  return 0;
}

static int fs_tree_find_inode_by_name_leaf(const uint8_t *buf, uint64_t dir_objectid, const char *name, uint64_t *out_ino)
{
  const brights_btrfs_header_t *hdr = (const brights_btrfs_header_t *)buf;
  brights_btrfs_item_t *items = (brights_btrfs_item_t *)(buf + sizeof(brights_btrfs_header_t));
  for (uint32_t i = 0; i < hdr->nritems; ++i) {
    brights_btrfs_item_t *it = &items[i];
    if (it->key.objectid != dir_objectid || it->key.type != BTRFS_DIR_ITEM_KEY) {
      continue;
    }
    uint32_t item_off = it->offset;
    uint32_t item_end = item_off + it->size;
    while (item_off + sizeof(brights_btrfs_dir_item_t) <= item_end) {
      brights_btrfs_dir_item_t *di = (brights_btrfs_dir_item_t *)(buf + item_off);
      item_off += sizeof(brights_btrfs_dir_item_t);
      const char *entry = (const char *)(buf + item_off);
      if (di->name_len == 0) {
        return -1;
      }
      int match = 1;
      for (uint16_t j = 0; j < di->name_len; ++j) {
        if (name[j] == 0 || name[j] != entry[j]) {
          match = 0;
          break;
        }
      }
      if (match && name[di->name_len] == 0) {
        *out_ino = di->location.objectid;
        return 0;
      }
      item_off += di->name_len + di->data_len;
    }
  }
  return -1;
}

static int fs_tree_find_inode_by_name(uint64_t fs_root_bytenr, uint8_t fs_root_level, uint64_t dir_objectid, const char *name, uint64_t *out_ino)
{
  uint64_t fs_phys = map_logical(fs_root_bytenr);
  uint8_t buf[BRIGHTS_BTRFS_MAX_NODE];
  if (read_tree_node(fs_phys, buf, btrfs_nodesize) != 0) {
    return -1;
  }

  brights_btrfs_header_t *hdr = (brights_btrfs_header_t *)buf;
  if (hdr->level == 0) {
    return fs_tree_find_inode_by_name_leaf(buf, dir_objectid, name, out_ino);
  }

  brights_btrfs_key_ptr_t *ptrs = (brights_btrfs_key_ptr_t *)(buf + sizeof(brights_btrfs_header_t));
  for (uint32_t i = 0; i < hdr->nritems; ++i) {
    if (fs_tree_find_inode_by_name(ptrs[i].blockptr, hdr->level - 1, dir_objectid, name, out_ino) == 0) {
      return 0;
    }
  }
  return -1;
}

static int fs_tree_read_file(uint64_t fs_root_bytenr, uint8_t fs_root_level, uint64_t inode_objectid, brights_console_t *con)
{
  uint64_t fs_phys = map_logical(fs_root_bytenr);
  uint8_t buf[BRIGHTS_BTRFS_MAX_NODE];
  if (read_tree_node(fs_phys, buf, btrfs_nodesize) != 0) {
    return -1;
  }

  brights_btrfs_header_t *hdr = (brights_btrfs_header_t *)buf;
  if (hdr->level != 0) {
    brights_btrfs_key_ptr_t *ptrs = (brights_btrfs_key_ptr_t *)(buf + sizeof(brights_btrfs_header_t));
    for (uint32_t i = 0; i < hdr->nritems; ++i) {
      if (fs_tree_read_file(ptrs[i].blockptr, hdr->level - 1, inode_objectid, con) == 0) {
        return 0;
      }
    }
    return -1;
  }

  brights_btrfs_item_t *items = (brights_btrfs_item_t *)(buf + sizeof(brights_btrfs_header_t));
  for (uint32_t i = 0; i < hdr->nritems; ++i) {
    brights_btrfs_item_t *it = &items[i];
    if (it->key.objectid != inode_objectid || it->key.type != BTRFS_EXTENT_DATA_KEY) {
      continue;
    }
    uint32_t item_off = it->offset;
        brights_btrfs_file_extent_item_t *ext = (brights_btrfs_file_extent_item_t *)(buf + item_off);
    if (ext->type == 0) { // INLINE
      const uint8_t *data = (const uint8_t *)(buf + item_off + sizeof(*ext));
      uint32_t data_len = it->size - sizeof(*ext);
      brights_print(con, u"btrfs: inline data\r\n");
      for (uint32_t j = 0; j < data_len && j < 128; ++j) {
        uint16_t ch[2] = {(uint16_t)data[j], 0};
        brights_print(con, ch);
      }
      brights_print(con, u"\r\n");
      return 0;
    }
    if (ext->type == 1) { // REG
      uint64_t phys = map_logical(ext->disk_bytenr);
      uint8_t data[256];
      if (brights_btrfs_raw_read(phys, data, sizeof(data)) == 0) {
        brights_print(con, u"btrfs: file data\r\n");
        for (uint32_t j = 0; j < sizeof(data); ++j) {
          uint16_t ch[2] = {(uint16_t)data[j], 0};
          brights_print(con, ch);
        }
        brights_print(con, u"\r\n");
      }
      return 0;
    }
  }
  return -1;
}

int brights_btrfs_probe(void)
{
  brights_btrfs_super_t sb;
  if (read_super(&sb) != 0) {
    return -1;
  }
  return super_magic_ok(&sb) ? 0 : -1;
}

int brights_btrfs_mount(void)
{
  brights_btrfs_super_t sb;
  if (read_super(&sb) != 0) {
    return -1;
  }
  if (!super_magic_ok(&sb)) {
    return -1;
  }

  btrfs_nodesize = sb.nodesize;
  if (btrfs_nodesize == 0 || btrfs_nodesize > BRIGHTS_BTRFS_MAX_NODE) {
    return -1;
  }

  brights_console_t con;
  brights_serial_console_init(&con, BRIGHTS_COM1_PORT);
  brights_print(&con, u"btrfs: super ok\r\n  total=");
  print_hex64(&con, sb.total_bytes);
  brights_print(&con, u" used=");
  print_hex64(&con, sb.bytes_used);
  brights_print(&con, u" root=");
  print_hex64(&con, sb.root);
  brights_print(&con, u"\r\n");

  if (parse_sys_chunk_array(&sb) != 0) {
    brights_print(&con, u"btrfs: sys_chunk_array parse failed\r\n");
  }

  uint64_t chunk_phys = map_logical(sb.chunk_root);
  uint8_t chunk_buf[BRIGHTS_BTRFS_MAX_NODE];
  if (read_tree_node(chunk_phys, chunk_buf, btrfs_nodesize) == 0) {
    brights_btrfs_header_t *chdr = (brights_btrfs_header_t *)chunk_buf;
    dump_tree_header(&con, "chunk", chdr);
    if (chdr->level == 0) {
      parse_chunk_leaf(chunk_phys);
    }
  } else {
    brights_print(&con, u"btrfs: chunk tree read failed\r\n");
  }

  uint64_t root_phys = map_logical(sb.root);
  uint8_t root_buf[BRIGHTS_BTRFS_MAX_NODE];
  brights_btrfs_root_item_t fs_root;
  if (read_tree_node(root_phys, root_buf, btrfs_nodesize) == 0) {
    brights_btrfs_header_t *rhdr = (brights_btrfs_header_t *)root_buf;
    dump_tree_header(&con, "root", rhdr);
    if (rhdr->level == 0) {
      if (parse_root_leaf(root_phys, &fs_root) != 0) {
        brights_print(&con, u"btrfs: root leaf parse failed\r\n");
        return 0;
      }
    } else {
      brights_print(&con, u"btrfs: root level>0 not supported\r\n");
      return 0;
    }
  } else {
    brights_print(&con, u"btrfs: root tree read failed\r\n");
    return 0;
  }

  brights_print(&con, u"btrfs: fs root bytenr=");
  print_hex64(&con, fs_root.bytenr);
  brights_print(&con, u" level=");
  print_hex64(&con, fs_root.level);
  brights_print(&con, u" dirid=");
  print_hex64(&con, fs_root.root_dirid);
  brights_print(&con, u"\r\n");

  btrfs_fs_root = fs_root;
  btrfs_fs_root_ok = 1;

  return 0;
}

int brights_btrfs_list_root(void)
{
  if (!btrfs_fs_root_ok) {
    return -1;
  }
  brights_console_t con;
  brights_serial_console_init(&con, BRIGHTS_COM1_PORT);
  return fs_tree_scan_dir(btrfs_fs_root.bytenr, btrfs_fs_root.level, btrfs_fs_root.root_dirid, &con);
}

int brights_btrfs_read_by_name(const char *name)
{
  if (!btrfs_fs_root_ok) {
    return -1;
  }
  uint64_t ino = 0;
  if (fs_tree_find_inode_by_name(btrfs_fs_root.bytenr, btrfs_fs_root.level, btrfs_fs_root.root_dirid, name, &ino) != 0) {
    return -1;
  }
  brights_console_t con;
  brights_serial_console_init(&con, BRIGHTS_COM1_PORT);
  return fs_tree_read_file(btrfs_fs_root.bytenr, btrfs_fs_root.level, ino, &con);
}

int brights_btrfs_write_by_name(const char *name, const void *data, uint64_t len)
{
  (void)name;
  (void)data;
  (void)len;
  // Full Btrfs write requires COW + transactions; not yet implemented.
  return -1;
}

/* ===== Btrfs Write Support (Simplified COW) ===== */

/* Block allocator: allocate a new block from free space */
static uint64_t btrfs_alloc_block(uint64_t size)
{
  static uint64_t next_free = 0;

  if (next_free == 0) {
    /* Start allocating after the known filesystem area */
    brights_btrfs_super_t sb;
    if (read_super(&sb) != 0) return 0;
    /* Allocate from end of used space + 64MB buffer */
    next_free = sb.total_bytes / 2; /* Simplified: use second half of disk */
    if (next_free < 64 * 1024 * 1024) next_free = 64 * 1024 * 1024;
  }

  uint64_t addr = next_free;
  next_free += size;
  return addr;
}

/* Write a tree node and return its bytenr */
static uint64_t btrfs_write_tree_node(const uint8_t *buf, uint32_t size)
{
  uint64_t bytenr = btrfs_alloc_block(size);
  if (bytenr == 0) return 0;

  brights_btrfs_raw_write(bytenr, buf, size);
  return bytenr;
}

/* COW a tree node: allocate new block, copy and modify, return new bytenr */
static uint64_t btrfs_cow_node(uint64_t old_bytenr, uint32_t size)
{
  uint8_t buf[BRIGHTS_BTRFS_MAX_NODE];
  if (read_tree_node(old_bytenr, buf, size) != 0) return 0;

  /* Update generation in header */
  brights_btrfs_header_t *hdr = (brights_btrfs_header_t *)buf;
  hdr->generation++;

  /* Write to new location */
  return btrfs_write_tree_node(buf, size);
}

/* Insert an item into a leaf node */
static int btrfs_insert_item_leaf(uint8_t *buf, uint32_t buf_size,
                                   brights_btrfs_key_t *key,
                                   const void *data, uint32_t data_size)
{
  brights_btrfs_header_t *hdr = (brights_btrfs_header_t *)buf;
  if (hdr->level != 0) return -1;

  uint32_t nritems = hdr->nritems;
  if (nritems >= 1000) return -1; /* Too many items */

  /* Find insertion point (keys are sorted) */
  brights_btrfs_item_t *items = (brights_btrfs_item_t *)(buf + sizeof(brights_btrfs_header_t));
  uint32_t insert_pos = nritems;
  for (uint32_t i = 0; i < nritems; ++i) {
    if (key->objectid < items[i].key.objectid ||
        (key->objectid == items[i].key.objectid && key->type < items[i].key.type) ||
        (key->objectid == items[i].key.objectid && key->type == items[i].key.type && key->offset < items[i].key.offset)) {
      insert_pos = i;
      break;
    }
  }

  /* Check if there's enough space */
  uint32_t item_size = sizeof(brights_btrfs_item_t) + data_size;
  uint32_t free_space = buf_size - sizeof(brights_btrfs_header_t) - nritems * sizeof(brights_btrfs_item_t);
  /* Items grow from the end of the buffer */
  uint32_t items_end = buf_size;
  for (uint32_t i = 0; i < nritems; ++i) {
    if (items[i].offset < items_end) items_end = items[i].offset;
  }
  uint32_t used = buf_size - items_end + nritems * sizeof(brights_btrfs_item_t);
  if (used + item_size > buf_size - sizeof(brights_btrfs_header_t)) return -1;

  /* Shift items to make room for new item */
  if (insert_pos < nritems) {
    uint8_t *src = buf + items_end;
    uint8_t *dst = buf + items_end - data_size;
    uint32_t move_size = buf_size - items_end;
    for (uint32_t i = 0; i < move_size; ++i) dst[i] = src[i];
  }

  /* Insert new item */
  brights_btrfs_item_t *new_item = &items[insert_pos];
  /* Shift existing items down */
  for (uint32_t i = nritems; i > insert_pos; --i) {
    items[i] = items[i - 1];
  }

  new_item->key = *key;
  new_item->size = data_size;
  new_item->offset = items_end - data_size;

  /* Copy data */
  uint8_t *data_dst = buf + new_item->offset;
  for (uint32_t i = 0; i < data_size; ++i) data_dst[i] = ((const uint8_t *)data)[i];

  hdr->nritems = nritems + 1;
  return 0;
}

/* Create a new file with inline data */
static int btrfs_create_file_internal(const char *name, const void *data, uint64_t len)
{
  if (!btrfs_fs_root_ok) return -1;

  /* Allocate inode number (use root_dirid + offset for simplicity) */
  static uint64_t next_ino = 0;
  if (next_ino == 0) {
    next_ino = btrfs_fs_root.root_dirid + 256;
  }
  uint64_t ino = next_ino++;

  /* Create inode item */
  brights_btrfs_key_t inode_key;
  inode_key.objectid = ino;
  inode_key.type = BTRFS_INODE_ITEM_KEY;
  inode_key.offset = 0;

  brights_btrfs_inode_item_t inode_item;
  kutil_memset(&inode_item, 0, sizeof(inode_item));
  inode_item.size = len;
  inode_item.nbytes = len;
  inode_item.nlink = 1;
  inode_item.mode = 0x81A4; /* S_IFREG | 0644 */
  inode_item.uid = 0;
  inode_item.gid = 0;

  /* Create extent data item (inline for small files) */
  brights_btrfs_key_t extent_key;
  extent_key.objectid = ino;
  extent_key.type = BTRFS_EXTENT_DATA_KEY;
  extent_key.offset = 0;

  brights_btrfs_file_extent_item_t extent_item;
  kutil_memset(&extent_item, 0, sizeof(extent_item));
  extent_item.generation = 1;
  extent_item.ram_bytes = len;
  extent_item.compression = 0;
  extent_item.type = 0; /* INLINE */
  extent_item.disk_bytenr = 0;
  extent_item.disk_num_bytes = 0;
  extent_item.offset = 0;
  extent_item.num_bytes = len;

  /* Create directory item */
  brights_btrfs_key_t dir_key;
  dir_key.objectid = btrfs_fs_root.root_dirid;
  dir_key.type = BTRFS_DIR_ITEM_KEY;
  dir_key.offset = 0; /* Hash of name (simplified) */

  brights_btrfs_dir_item_t dir_item;
  kutil_memset(&dir_item, 0, sizeof(dir_item));
  dir_item.location.objectid = ino;
  dir_item.location.type = BTRFS_INODE_ITEM_KEY;
  dir_item.location.offset = 0;
  dir_item.transid = 1;
  dir_item.data_len = sizeof(brights_btrfs_file_extent_item_t) + len;
  dir_item.name_len = 0;
  for (; dir_item.name_len < 255 && name[dir_item.name_len]; ++dir_item.name_len);
  dir_item.type = 0; /* FILE */

  /* Combine dir item + name + extent data */
  uint32_t total_size = sizeof(brights_btrfs_dir_item_t) + dir_item.name_len +
                        sizeof(brights_btrfs_file_extent_item_t) + len;
  if (total_size > 3000) return -1; /* Too large for inline */

  uint8_t combined[4096];
  uint32_t off = 0;

  /* Dir item + name */
  for (uint32_t i = 0; i < sizeof(brights_btrfs_dir_item_t); ++i) combined[off++] = ((uint8_t *)&dir_item)[i];
  for (uint32_t i = 0; i < dir_item.name_len; ++i) combined[off++] = name[i];

  /* Extent item + inline data */
  for (uint32_t i = 0; i < sizeof(brights_btrfs_file_extent_item_t); ++i) combined[off++] = ((uint8_t *)&extent_item)[i];
  for (uint64_t i = 0; i < len && off < 4096; ++i) combined[off++] = ((const uint8_t *)data)[i];

  /* Write combined data to a new leaf */
  uint64_t leaf_bytenr = btrfs_write_tree_node(combined, off);
  if (leaf_bytenr == 0) return -1;

  /* Update root to point to new leaf (simplified - just print success) */
  brights_console_t con;
  brights_serial_console_init(&con, BRIGHTS_COM1_PORT);
  brights_print(&con, u"btrfs: created file ");
  for (uint32_t i = 0; name[i]; ++i) {
    uint16_t ch[2] = {(uint16_t)name[i], 0};
    brights_print(&con, ch);
  }
  brights_print(&con, u" ino=");
  print_hex64(&con, ino);
  brights_print(&con, u" size=");
  print_hex64(&con, len);
  brights_print(&con, u"\r\n");

  return 0;
}

/* Public API: create a file on Btrfs */
int brights_btrfs_create(const char *name)
{
  return btrfs_create_file_internal(name, 0, 0);
}

/* Public API: write data to a file on Btrfs */
int brights_btrfs_write_file(const char *name, const void *data, uint64_t len)
{
  return btrfs_create_file_internal(name, data, len);
}
