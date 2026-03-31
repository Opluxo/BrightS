#ifndef BRIGHTS_V6FS_H
#define BRIGHTS_V6FS_H

#include <stdint.h>

// UNIX V6-inspired file system structures
#define BRIGHTS_V6FS_MAX_INODES 256
#define BRIGHTS_V6FS_MAX_BLOCKS 1024
#define BRIGHTS_V6FS_BLOCK_SIZE 512
#define BRIGHTS_V6FS_NAME_LEN 14

// File types
#define BRIGHTS_V6FS_TYPE_FREE   0
#define BRIGHTS_V6FS_TYPE_FILE   1
#define BRIGHTS_V6FS_TYPE_DIR    2
#define BRIGHTS_V6FS_TYPE_CHAR   3
#define BRIGHTS_V6FS_TYPE_BLOCK  4

// Inode structure (simplified V6)
typedef struct {
  uint16_t mode;        // File type and permissions
  uint16_t nlinks;      // Number of links
  uint16_t uid;         // Owner user ID
  uint16_t gid;         // Owner group ID
  uint32_t size;        // File size in bytes
  uint32_t addrs[8];    // Direct block addresses (7 direct + 1 indirect)
  uint32_t atime;       // Access time
  uint32_t mtime;       // Modification time
  uint32_t ctime;       // Creation time
} brights_v6fs_inode_t;

// Directory entry
typedef struct {
  uint16_t ino;                    // Inode number
  char name[BRIGHTS_V6FS_NAME_LEN]; // File name
} brights_v6fs_dirent_t;

// Initialize V6 file system
void brights_v6fs_init(void);

// Format V6 file system
int brights_v6fs_format(void);

// Allocate a new inode
int brights_v6fs_alloc_inode(void);

// Free an inode
void brights_v6fs_free_inode(int ino);

// Read inode
int brights_v6fs_read_inode(int ino, brights_v6fs_inode_t *out);

// Write inode
int brights_v6fs_write_inode(int ino, const brights_v6fs_inode_t *inode);

// Allocate a new block
int brights_v6fs_alloc_block(void);

// Free a block
void brights_v6fs_free_block(int blkno);

// Create a file
int brights_v6fs_create(const char *name, uint16_t mode);

// Remove a file
int brights_v6fs_remove(const char *name);

// Read from file
int brights_v6fs_read(int ino, uint32_t offset, void *buf, uint32_t len);

// Write to file
int brights_v6fs_write(int ino, uint32_t offset, const void *buf, uint32_t len);

// List directory
int brights_v6fs_ls(const char *path);

#endif
