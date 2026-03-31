#include "v6fs.h"
#include <stdint.h>

// In-memory inode and block storage
static brights_v6fs_inode_t inode_table[BRIGHTS_V6FS_MAX_INODES];
static uint8_t block_storage[BRIGHTS_V6FS_MAX_BLOCKS][BRIGHTS_V6FS_BLOCK_SIZE];
static uint8_t inode_bitmap[(BRIGHTS_V6FS_MAX_INODES + 7) / 8];
static uint8_t block_bitmap[(BRIGHTS_V6FS_MAX_BLOCKS + 7) / 8];

// Bitmap helpers
static int bitmap_test(const uint8_t *bmp, int idx)
{
  return (bmp[idx / 8] >> (idx % 8)) & 1;
}

static void bitmap_set(uint8_t *bmp, int idx)
{
  bmp[idx / 8] |= (1 << (idx % 8));
}

static void bitmap_clear(uint8_t *bmp, int idx)
{
  bmp[idx / 8] &= ~(1 << (idx % 8));
}

// Initialize V6 file system
void brights_v6fs_init(void)
{
  // Clear all structures
  for (int i = 0; i < BRIGHTS_V6FS_MAX_INODES; ++i) {
    inode_table[i].mode = BRIGHTS_V6FS_TYPE_FREE;
  }
  for (int i = 0; i < (int)sizeof(inode_bitmap); ++i) {
    inode_bitmap[i] = 0;
  }
  for (int i = 0; i < (int)sizeof(block_bitmap); ++i) {
    block_bitmap[i] = 0;
  }
  
  // Reserve inode 0 (invalid) and 1 (root directory)
  bitmap_set(inode_bitmap, 0);
  bitmap_set(inode_bitmap, 1);
  
  // Initialize root directory inode
  inode_table[1].mode = BRIGHTS_V6FS_TYPE_DIR | 0755;
  inode_table[1].nlinks = 2;
  inode_table[1].uid = 0;
  inode_table[1].gid = 0;
  inode_table[1].size = 0;
  for (int i = 0; i < 8; ++i) {
    inode_table[1].addrs[i] = 0;
  }
}

// Format file system
int brights_v6fs_format(void)
{
  brights_v6fs_init();
  return 0;
}

// Allocate inode
int brights_v6fs_alloc_inode(void)
{
  for (int i = 1; i < BRIGHTS_V6FS_MAX_INODES; ++i) {
    if (!bitmap_test(inode_bitmap, i)) {
      bitmap_set(inode_bitmap, i);
      // Clear inode
      inode_table[i].mode = BRIGHTS_V6FS_TYPE_FILE | 0644;
      inode_table[i].nlinks = 1;
      inode_table[i].uid = 0;
      inode_table[i].gid = 0;
      inode_table[i].size = 0;
      for (int j = 0; j < 8; ++j) {
        inode_table[i].addrs[j] = 0;
      }
      return i;
    }
  }
  return -1; // No free inodes
}

// Free inode
void brights_v6fs_free_inode(int ino)
{
  if (ino < 1 || ino >= BRIGHTS_V6FS_MAX_INODES) {
    return;
  }
  
  // Free associated blocks
  for (int i = 0; i < 7; ++i) {
    if (inode_table[ino].addrs[i] != 0) {
      brights_v6fs_free_block(inode_table[ino].addrs[i]);
    }
  }
  
  inode_table[ino].mode = BRIGHTS_V6FS_TYPE_FREE;
  bitmap_clear(inode_bitmap, ino);
}

// Read inode
int brights_v6fs_read_inode(int ino, brights_v6fs_inode_t *out)
{
  if (ino < 0 || ino >= BRIGHTS_V6FS_MAX_INODES || !out) {
    return -1;
  }
  if (!bitmap_test(inode_bitmap, ino)) {
    return -1;
  }
  *out = inode_table[ino];
  return 0;
}

// Write inode
int brights_v6fs_write_inode(int ino, const brights_v6fs_inode_t *inode)
{
  if (ino < 0 || ino >= BRIGHTS_V6FS_MAX_INODES || !inode) {
    return -1;
  }
  if (!bitmap_test(inode_bitmap, ino)) {
    return -1;
  }
  inode_table[ino] = *inode;
  return 0;
}

// Allocate block
int brights_v6fs_alloc_block(void)
{
  for (int i = 0; i < BRIGHTS_V6FS_MAX_BLOCKS; ++i) {
    if (!bitmap_test(block_bitmap, i)) {
      bitmap_set(block_bitmap, i);
      // Clear block
      for (int j = 0; j < BRIGHTS_V6FS_BLOCK_SIZE; ++j) {
        block_storage[i][j] = 0;
      }
      return i;
    }
  }
  return -1; // No free blocks
}

// Free block
void brights_v6fs_free_block(int blkno)
{
  if (blkno < 0 || blkno >= BRIGHTS_V6FS_MAX_BLOCKS) {
    return;
  }
  bitmap_clear(block_bitmap, blkno);
}

// Create file
int brights_v6fs_create(const char *name, uint16_t mode)
{
  (void)name;
  
  int ino = brights_v6fs_alloc_inode();
  if (ino < 0) {
    return -1;
  }
  
  inode_table[ino].mode = mode;
  return ino;
}

// Remove file
int brights_v6fs_remove(const char *name)
{
  (void)name;
  // Simplified: just find and free
  // Full implementation would search directory
  return -1;
}

// Read from file
int brights_v6fs_read(int ino, uint32_t offset, void *buf, uint32_t len)
{
  if (ino < 1 || ino >= BRIGHTS_V6FS_MAX_INODES || !buf) {
    return -1;
  }
  if (!bitmap_test(inode_bitmap, ino)) {
    return -1;
  }
  
  brights_v6fs_inode_t *ip = &inode_table[ino];
  if (offset >= ip->size) {
    return 0;
  }
  
  if (offset + len > ip->size) {
    len = ip->size - offset;
  }
  
  uint8_t *dst = (uint8_t *)buf;
  uint32_t copied = 0;
  
  while (copied < len) {
    uint32_t blk_idx = (offset + copied) / BRIGHTS_V6FS_BLOCK_SIZE;
    uint32_t blk_off = (offset + copied) % BRIGHTS_V6FS_BLOCK_SIZE;
    
    if (blk_idx >= 7) {
      // Indirect blocks not implemented yet
      break;
    }
    
    uint32_t blkno = ip->addrs[blk_idx];
    if (blkno == 0) {
      break;
    }
    
    uint32_t to_copy = BRIGHTS_V6FS_BLOCK_SIZE - blk_off;
    if (to_copy > len - copied) {
      to_copy = len - copied;
    }
    
    for (uint32_t i = 0; i < to_copy; ++i) {
      dst[copied + i] = block_storage[blkno][blk_off + i];
    }
    copied += to_copy;
  }
  
  return (int)copied;
}

// Write to file
int brights_v6fs_write(int ino, uint32_t offset, const void *buf, uint32_t len)
{
  if (ino < 1 || ino >= BRIGHTS_V6FS_MAX_INODES || !buf) {
    return -1;
  }
  if (!bitmap_test(inode_bitmap, ino)) {
    return -1;
  }
  
  brights_v6fs_inode_t *ip = &inode_table[ino];
  const uint8_t *src = (const uint8_t *)buf;
  uint32_t written = 0;
  
  while (written < len) {
    uint32_t blk_idx = (offset + written) / BRIGHTS_V6FS_BLOCK_SIZE;
    uint32_t blk_off = (offset + written) % BRIGHTS_V6FS_BLOCK_SIZE;
    
    if (blk_idx >= 7) {
      // Indirect blocks not implemented yet
      break;
    }
    
    // Allocate block if needed
    if (ip->addrs[blk_idx] == 0) {
      int blkno = brights_v6fs_alloc_block();
      if (blkno < 0) {
        break;
      }
      ip->addrs[blk_idx] = (uint32_t)blkno;
    }
    
    uint32_t blkno = ip->addrs[blk_idx];
    uint32_t to_write = BRIGHTS_V6FS_BLOCK_SIZE - blk_off;
    if (to_write > len - written) {
      to_write = len - written;
    }
    
    for (uint32_t i = 0; i < to_write; ++i) {
      block_storage[blkno][blk_off + i] = src[written + i];
    }
    written += to_write;
  }
  
  // Update file size
  if (offset + written > ip->size) {
    ip->size = offset + written;
  }
  
  return (int)written;
}

// List directory (simplified)
int brights_v6fs_ls(const char *path)
{
  (void)path;
  // Simplified: just show root inodes
  for (int i = 1; i < BRIGHTS_V6FS_MAX_INODES; ++i) {
    if (bitmap_test(inode_bitmap, i)) {
      // Would print inode info
    }
  }
  return 0;
}
