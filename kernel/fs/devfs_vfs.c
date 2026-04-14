#ifndef BRIGHTS_DEVFS_VFS_H
#define BRIGHTS_DEVFS_VFS_H

#include "vfs2.h"

/* Get the devfs VFS superblock */
vfs_superblock_t *brights_devfs_vfs_sb(void);

/* Initialize devfs devices */
void brights_devfs_vfs_init(void);

#endif
#include "vfs2.h"
#include "../drivers/tty.h"
#include "../drivers/serial.h"
#include "../drivers/rtc.h"
#include "../drivers/ps2kbd.h"
#include "../drivers/block.h"
#include <stdint.h>

/* ---- Device nodes ---- */
#define DEVFS_MAX_DEVICES 16

typedef struct {
  const char *name;
  uint32_t ino;
  uint32_t mode;    /* VFS_S_IFCHR or VFS_S_IFBLK */
  vfs_inode_t inode;
} devfs_node_t;

static devfs_node_t devfs_nodes[DEVFS_MAX_DEVICES];
static int devfs_count = 0;
static vfs_superblock_t devfs_sb;

static void devfs_add(const char *name, uint32_t ino, uint32_t mode)
{
  if (devfs_count >= DEVFS_MAX_DEVICES) return;
  devfs_node_t *n = &devfs_nodes[devfs_count];
  n->name = name;
  n->ino = ino;
  n->mode = mode;
  n->inode.ino = ino;
  n->inode.mode = mode;
  n->inode.size = 0;
  n->inode.sb = &devfs_sb;
  n->inode.fs_private = (void *)(uintptr_t)devfs_count;
  devfs_count++;
}




/* ---- file_ops tables ---- */


static int64_t devfs_tty_read(vfs_file_t *f, void *buf, uint64_t count)
{
  (void)f;
  char *dst = (char *)buf;
  for (uint64_t i = 0; i < count; ++i) {
    dst[i] = brights_tty_read_char_blocking();
  }
  return (int64_t)count;
}

static int64_t devfs_tty_write(vfs_file_t *f, const void *buf, uint64_t count)
{
  (void)f;
  const char *src = (const char *)buf;
  for (uint64_t i = 0; i < count; ++i) {
    char ch[2] = {src[i], 0};
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, ch);
  }
  return (int64_t)count;
}

static vfs_file_ops_t devfs_tty_full_fops = {
  .read = devfs_tty_read,
  .write = devfs_tty_write,
  .close = 0,
  .lseek = 0,
  .ioctl = 0,
  .readdir = 0,
};



/* ---- sb_ops ---- */
static vfs_inode_t *devfs_sb_lookup(vfs_superblock_t *sb, const char *path)
{
  (void)sb;
  if (!path) return 0;

  /* Root of devfs */
  if (path[0] == '/' && path[1] == 0) {
    static vfs_inode_t devfs_root_inode;
    devfs_root_inode.ino = 0;
    devfs_root_inode.mode = VFS_S_IFDIR;
    devfs_root_inode.size = 0;
    devfs_root_inode.sb = &devfs_sb;
    devfs_root_inode.fs_private = (void *)(uintptr_t)-1;
    return &devfs_root_inode;
  }

  /* Skip leading '/' */
  const char *name = path;
  if (*name == '/') ++name;

  for (int i = 0; i < devfs_count; ++i) {
    int match = 1;
    const char *a = devfs_nodes[i].name;
    const char *b = name;
    while (*a && *b && *a == *b) { ++a; ++b; }
    if (*a != 0 || *b != 0) match = 0;
    if (match) return &devfs_nodes[i].inode;
  }
  return 0;
}

static int devfs_sb_create(vfs_superblock_t *sb, const char *path)
{
  (void)sb; (void)path;
  return -1;
}

static int devfs_sb_mkdir(vfs_superblock_t *sb, const char *path)
{
  (void)sb; (void)path;
  return -1;
}

static int devfs_sb_unlink(vfs_superblock_t *sb, const char *path)
{
  (void)sb; (void)path;
  return -1;
}

static int devfs_sb_stat(vfs_superblock_t *sb, const char *path,
                         uint64_t *size_out, uint32_t *mode_out)
{
  (void)sb;
  vfs_inode_t *ino = devfs_sb_lookup(&devfs_sb, path);
  if (!ino) return -1;
  if (size_out) *size_out = ino->size;
  if (mode_out) *mode_out = ino->mode;
  return 0;
}

static int devfs_sb_readdir(vfs_superblock_t *sb, const char *dir_path,
                            char *name_buf, uint64_t buf_size)
{
  (void)sb;
  if (dir_path[0] != '/' || dir_path[1] != 0) return -1;

  int written = 0;
  for (int i = 0; i < devfs_count; ++i) {
    int nl = 0;
    while (devfs_nodes[i].name[nl]) ++nl;
    if (written + nl + 2 > (int)buf_size) break;
    if (written > 0) name_buf[written++] = '\n';
    for (int j = 0; j < nl; ++j) name_buf[written++] = devfs_nodes[i].name[j];
  }
  return written;
}

static vfs_sb_ops_t devfs_sb_ops = {
  .lookup = devfs_sb_lookup,
  .create = devfs_sb_create,
  .mkdir = devfs_sb_mkdir,
  .unlink = devfs_sb_unlink,
  .stat = devfs_sb_stat,
  .readdir = devfs_sb_readdir,
};


/* ---- Public ---- */
vfs_superblock_t *brights_devfs_vfs_sb(void)
{
  devfs_sb.fs_type_name = "devfs";
  devfs_sb.sb_ops = &devfs_sb_ops;
  devfs_sb.default_fops = &devfs_tty_full_fops;
  devfs_sb.fs_private = 0;
  devfs_sb.active = 1;
  return &devfs_sb;
}

void brights_devfs_vfs_init(void)
{
  devfs_count = 0;
  devfs_add("tty0",  1, VFS_S_IFCHR);  /* Console terminal */
  devfs_add("null",  2, VFS_S_IFCHR);  /* Null device */
  devfs_add("zero",  3, VFS_S_IFCHR);  /* Zero device */
  devfs_add("rtc",   4, VFS_S_IFCHR);  /* Real-time clock */
  devfs_add("kbd",   5, VFS_S_IFCHR);  /* Keyboard */
  devfs_add("disk0", 6, VFS_S_IFBLK);  /* Block device */
}
