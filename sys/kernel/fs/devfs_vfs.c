#ifndef BRIGHTS_DEVFS_VFS_H
#define BRIGHTS_DEVFS_VFS_H

#include "vfs2.h"

/* Get the devfs VFS superblock */
vfs_superblock_t *brights_devfs_vfs_sb(void);

/* Initialize devfs devices */
void brights_devfs_vfs_init(void);

#endif
#include "vfs2.h"
#include "../dev/tty.h"
#include "../dev/serial.h"
#include "../dev/rtc.h"
#include "../dev/ps2kbd.h"
#include "../dev/block.h"
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

/* ---- null device: /dev/null ---- */
static int64_t devfs_null_read(vfs_file_t *f, void *buf, uint64_t count)
{
  (void)f; (void)buf;
  (void)count;
  return 0; /* EOF */
}

static int64_t devfs_null_write(vfs_file_t *f, const void *buf, uint64_t count)
{
  (void)f; (void)buf;
  return (int64_t)count; /* Discard */
}

/* ---- zero device: /dev/zero ---- */
static int64_t devfs_zero_read(vfs_file_t *f, void *buf, uint64_t count)
{
  (void)f;
  uint8_t *dst = (uint8_t *)buf;
  for (uint64_t i = 0; i < count; ++i) dst[i] = 0;
  return (int64_t)count;
}

static int64_t devfs_zero_write(vfs_file_t *f, const void *buf, uint64_t count)
{
  (void)f; (void)buf;
  return (int64_t)count;
}

/* ---- rtc device: /dev/rtc ---- */
static int64_t devfs_rtc_read(vfs_file_t *f, void *buf, uint64_t count)
{
  (void)f;
  if (count < sizeof(brights_rtc_time_t)) return -1;
  return brights_rtc_read((brights_rtc_time_t *)buf) == 0 ? (int64_t)sizeof(brights_rtc_time_t) : -1;
}

/* ---- kbd device: /dev/kbd ---- */
static int64_t devfs_kbd_read(vfs_file_t *f, void *buf, uint64_t count)
{
  (void)f;
  char *dst = (char *)buf;
  for (uint64_t i = 0; i < count; ++i) {
    dst[i] = brights_tty_read_char_blocking();
  }
  return (int64_t)count;
}

/* ---- disk device: block I/O via ioctl ---- */
/* ioctl commands for block devices */
#define IOCTL_DISK_SIZE   0x1001
#define IOCTL_DISK_READ   0x1002
#define IOCTL_DISK_WRITE  0x1003

static int devfs_disk_ioctl(vfs_file_t *f, uint64_t request, void *argp)
{
  (void)f;
  brights_block_dev_t *dev = brights_block_root();
  if (!dev) return -1;

  switch (request) {
    case IOCTL_DISK_SIZE: {
      /* Return block count (placeholder: 0) */
      uint64_t *size = (uint64_t *)argp;
      if (size) *size = 0;
      return 0;
    }
    case IOCTL_DISK_READ: {
      /* argp points to struct { uint64_t lba; void *buf; uint64_t count; } */
      struct { uint64_t lba; void *buf; uint64_t count; } *req = argp;
      if (!req || !dev->read) return -1;
      return dev->read(req->lba, req->buf, req->count);
    }
    case IOCTL_DISK_WRITE: {
      struct { uint64_t lba; const void *buf; uint64_t count; } *req = argp;
      if (!req || !dev->write) return -1;
      return dev->write(req->lba, req->buf, req->count);
    }
    default:
      return -1;
  }
}

/* ---- file_ops tables ---- */

/* tty0: stdin/stdout */
static vfs_file_ops_t devfs_tty_fops = {
  .read = 0,
  .write = 0,
  .close = 0,
  .lseek = 0,
  .ioctl = 0,
  .readdir = 0,
};

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

/* null device */
static vfs_file_ops_t devfs_null_fops = {
  .read = devfs_null_read,
  .write = devfs_null_write,
  .close = 0,
  .lseek = 0,
  .ioctl = 0,
  .readdir = 0,
};

/* zero device */
static vfs_file_ops_t devfs_zero_fops = {
  .read = devfs_zero_read,
  .write = devfs_zero_write,
  .close = 0,
  .lseek = 0,
  .ioctl = 0,
  .readdir = 0,
};

/* rtc device */
static vfs_file_ops_t devfs_rtc_fops = {
  .read = devfs_rtc_read,
  .write = 0,
  .close = 0,
  .lseek = 0,
  .ioctl = 0,
  .readdir = 0,
};

/* kbd device */
static vfs_file_ops_t devfs_kbd_fops = {
  .read = devfs_kbd_read,
  .write = 0,
  .close = 0,
  .lseek = 0,
  .ioctl = 0,
  .readdir = 0,
};

/* disk device */
static vfs_file_ops_t devfs_disk_fops = {
  .read = 0,
  .write = 0,
  .close = 0,
  .lseek = 0,
  .ioctl = devfs_disk_ioctl,
  .readdir = 0,
};

/* ---- Get fops for a device ---- */
static vfs_file_ops_t *devfs_get_fops(int idx)
{
  if (idx < 0 || idx >= devfs_count) return 0;
  const char *name = devfs_nodes[idx].name;

  /* Match by device name */
  if (name[0] == 't' && name[1] == 't' && name[2] == 'y') return &devfs_tty_full_fops;
  if (name[0] == 'n' && name[1] == 'u' && name[2] == 'l' && name[3] == 'l') return &devfs_null_fops;
  if (name[0] == 'z' && name[1] == 'e' && name[2] == 'r' && name[3] == 'o') return &devfs_zero_fops;
  if (name[0] == 'r' && name[1] == 't' && name[2] == 'c') return &devfs_rtc_fops;
  if (name[0] == 'k' && name[1] == 'b' && name[2] == 'd') return &devfs_kbd_fops;
  if (devfs_nodes[idx].mode == VFS_S_IFBLK) return &devfs_disk_fops;
  return &devfs_tty_full_fops;
}

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

/* ---- Open override ---- */
static int devfs_vfs_open(const char *path, uint32_t flags, vfs_file_t **out)
{
  (void)flags;
  if (!path || !out) return -1;

  vfs_inode_t *ino = devfs_sb_lookup(&devfs_sb, path);
  if (!ino) return -1;

  int idx = (int)(uintptr_t)ino->fs_private;
  if (idx < 0) return -1;

  vfs_file_t *f = brights_vfs2_file_alloc();
  if (!f) return -1;

  f->inode = ino;
  f->pos = 0;
  f->flags = flags;
  f->fops = devfs_get_fops(idx);
  f->private_data = ino->fs_private;

  *out = f;
  return 0;
}

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
