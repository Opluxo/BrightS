#ifndef BRIGHTS_RAMFS_VFS_H
#define BRIGHTS_RAMFS_VFS_H

#include "vfs2.h"

/* Get the ramfs VFS superblock */
vfs_superblock_t *brights_ramfs_vfs_sb(void);

/* Initialize ramfs VFS inode table */
void brights_ramfs_vfs_init(void);

#endif
#include "vfs2.h"
#include "ramfs.h"
#include <stdint.h>

/* ---- VFS inode wrapper for ramfs ---- */
#define RAMFS_VFS_MAX_INODES 64

static vfs_inode_t ramfs_vfs_inodes[RAMFS_VFS_MAX_INODES];

static vfs_inode_t *ramfs_vfs_inode_get(int ramfs_idx)
{
  if (ramfs_idx < 0 || ramfs_idx >= RAMFS_VFS_MAX_INODES) return 0;
  vfs_inode_t *ino = &ramfs_vfs_inodes[ramfs_idx];
  if (ino->mode == 0) return 0;
  return ino;
}

/* ---- file_ops: read/write/close/lseek ---- */
static int64_t ramfs_vfs_read(vfs_file_t *f, void *buf, uint64_t count)
{
  if (!f || !f->inode) return -1;
  int idx = (int)(uintptr_t)f->private_data;
  int64_t n = brights_ramfs_read(idx, f->pos, buf, count);
  if (n > 0) f->pos += (uint64_t)n;
  return n;
}

static int64_t ramfs_vfs_write(vfs_file_t *f, const void *buf, uint64_t count)
{
  if (!f || !f->inode) return -1;
  int idx = (int)(uintptr_t)f->private_data;
  int64_t n = brights_ramfs_write(idx, f->pos, buf, count);
  if (n > 0) {
    f->pos += (uint64_t)n;
    /* Update inode size */
    if (f->inode) {
      uint64_t sz = brights_ramfs_file_size(idx);
      if (sz > f->inode->size) f->inode->size = sz;
    }
  }
  return n;
}

static int ramfs_vfs_close(vfs_file_t *f)
{
  (void)f;
  return 0;
}

static int64_t ramfs_vfs_lseek(vfs_file_t *f, int64_t offset, int whence)
{
  if (!f || !f->inode) return -1;
  switch (whence) {
    case VFS_SEEK_SET: f->pos = (uint64_t)offset; break;
    case VFS_SEEK_CUR: f->pos = (uint64_t)((int64_t)f->pos + offset); break;
    case VFS_SEEK_END: f->pos = (uint64_t)((int64_t)f->inode->size + offset); break;
    default: return -1;
  }
  return (int64_t)f->pos;
}

static int ramfs_vfs_readdir(vfs_file_t *f, char *name_buf, uint64_t buf_size)
{
  if (!f || !f->inode) return -1;
  int idx = (int)(uintptr_t)f->private_data;
  if (!brights_ramfs_is_dir_fd(idx)) return -1;

  /* Walk ramfs entries that are children of this directory */
  const char *dir_name = brights_ramfs_name_at(idx);
  if (!dir_name) return -1;
  int dir_len = 0;
  while (dir_name[dir_len]) ++dir_len;

  int written = 0;
  int entry = (int)f->pos;

  int total = brights_ramfs_count();
  for (int i = entry; i < total; ++i) {
    const char *name = brights_ramfs_name_at(i);
    if (!name) continue;
    if (!brights_ramfs_is_dir_fd(i) && !brights_ramfs_file_size(i)) {
      /* Skip non-dir with zero size? No, keep going */
    }

    /* Check if this entry is a direct child of dir_name */
    int is_child = 0;
    if (dir_len == 1 && dir_name[0] == '/') {
      /* Root: direct children have exactly one '/' */
      int slashes = 0;
      for (int j = 0; name[j]; ++j) if (name[j] == '/') ++slashes;
      if (slashes == 1 && name[0] == '/') is_child = 1;
    } else {
      /* Check prefix matches dir, and next char is '/' then name, no more '/' */
      int match = 1;
      for (int j = 0; j < dir_len; ++j) {
        if (name[j] != dir_name[j]) { match = 0; break; }
      }
      if (match && name[dir_len] == '/') {
        int extra_slashes = 0;
        for (int j = dir_len + 1; name[j]; ++j) {
          if (name[j] == '/') { extra_slashes = 1; break; }
        }
        if (!extra_slashes) is_child = 1;
      }
    }

    if (!is_child) continue;

    /* Extract basename */
    const char *base = name;
    for (int j = dir_len; name[j]; ++j) {
      if (name[j] == '/') base = &name[j + 1];
    }

    /* Write to buffer */
    int blen = 0;
    while (base[blen]) ++blen;
    if (written + blen + 2 > (int)buf_size) break;

    if (written > 0) name_buf[written++] = '\n';
    for (int j = 0; j < blen; ++j) name_buf[written++] = base[j];
    if (brights_ramfs_is_dir_fd(i)) name_buf[written++] = '/';
    f->pos = (uint64_t)(i + 1);
  }

  return written;
}

static vfs_file_ops_t ramfs_file_ops = {
  .read = ramfs_vfs_read,
  .write = ramfs_vfs_write,
  .close = ramfs_vfs_close,
  .lseek = ramfs_vfs_lseek,
  .ioctl = 0,
  .readdir = ramfs_vfs_readdir,
};

/* ---- sb_ops: lookup/create/mkdir/unlink/stat ---- */

/* Superblock private data: pointer to the ramfs superblock */
static vfs_superblock_t ramfs_sb;

static vfs_inode_t *ramfs_sb_lookup(vfs_superblock_t *sb, const char *path)
{
  (void)sb;
  if (!path) return 0;

  /* Root "/" */
  if (path[0] == '/' && path[1] == 0) {
    /* Return a synthetic root inode at slot 0 */
    ramfs_vfs_inodes[0].ino = 0;
    ramfs_vfs_inodes[0].mode = VFS_S_IFDIR;
    ramfs_vfs_inodes[0].size = 0;
    ramfs_vfs_inodes[0].sb = &ramfs_sb;
    ramfs_vfs_inodes[0].fs_private = (void *)(uintptr_t)0;
    return &ramfs_vfs_inodes[0];
  }

  int idx = brights_ramfs_open(path);
  if (idx < 0) return 0;

  if (idx >= RAMFS_VFS_MAX_INODES) return 0;

  vfs_inode_t *ino = &ramfs_vfs_inodes[idx];
  ino->ino = (uint32_t)idx;
  ino->mode = brights_ramfs_is_dir_fd(idx) ? VFS_S_IFDIR : VFS_S_IFREG;
  ino->size = brights_ramfs_file_size(idx);
  ino->sb = &ramfs_sb;
  ino->fs_private = (void *)(uintptr_t)idx;
  return ino;
}

static int ramfs_sb_create(vfs_superblock_t *sb, const char *path)
{
  (void)sb;
  return brights_ramfs_create(path);
}

static int ramfs_sb_mkdir(vfs_superblock_t *sb, const char *path)
{
  (void)sb;
  return brights_ramfs_mkdir(path);
}

static int ramfs_sb_unlink(vfs_superblock_t *sb, const char *path)
{
  (void)sb;
  return brights_ramfs_unlink(path);
}

static int ramfs_sb_stat(vfs_superblock_t *sb, const char *path,
                         uint64_t *size_out, uint32_t *mode_out)
{
  (void)sb;
  brights_ramfs_stat_t st;
  if (brights_ramfs_stat(path, &st) < 0) return -1;
  if (size_out) *size_out = st.size;
  if (mode_out) *mode_out = st.is_dir ? VFS_S_IFDIR : VFS_S_IFREG;
  return 0;
}

static int ramfs_sb_readdir(vfs_superblock_t *sb, const char *dir_path,
                            char *name_buf, uint64_t buf_size)
{
  (void)sb;
  int idx = brights_ramfs_open(dir_path);
  if (idx < 0) return -1;
  if (!brights_ramfs_is_dir_fd(idx)) return -1;

  vfs_file_t tmp;
  tmp.inode = 0;
  tmp.pos = 0;
  tmp.flags = 0;
  tmp.fops = &ramfs_file_ops;
  tmp.private_data = (void *)(uintptr_t)idx;
  tmp.in_use = 1;

  /* Find inode for this directory */
  vfs_inode_t *ino = ramfs_sb_lookup(&ramfs_sb, dir_path);
  if (!ino) return -1;
  tmp.inode = ino;

  return ramfs_vfs_readdir(&tmp, name_buf, buf_size);
}

static int ramfs_sb_symlink(vfs_superblock_t *sb, const char *target, const char *linkpath)
{
  (void)sb;
  return brights_ramfs_symlink(target, linkpath);
}

static int64_t ramfs_sb_readlink(vfs_superblock_t *sb, const char *path, char *buf, uint64_t bufsize)
{
  (void)sb;
  return brights_ramfs_readlink(path, buf, bufsize);
}

static int ramfs_sb_chmod(vfs_superblock_t *sb, const char *path, uint32_t mode)
{
  (void)sb;
  return brights_ramfs_chmod(path, mode);
}

static int ramfs_sb_chown(vfs_superblock_t *sb, const char *path, uint32_t uid, uint32_t gid)
{
  (void)sb;
  return brights_ramfs_chown(path, uid, gid);
}

static vfs_sb_ops_t ramfs_sb_ops = {
  .lookup = ramfs_sb_lookup,
  .create = ramfs_sb_create,
  .mkdir = ramfs_sb_mkdir,
  .unlink = ramfs_sb_unlink,
  .stat = ramfs_sb_stat,
  .readdir = ramfs_sb_readdir,
  .symlink = ramfs_sb_symlink,
  .readlink = ramfs_sb_readlink,
  .chmod = ramfs_sb_chmod,
  .chown = ramfs_sb_chown,
};

/* ---- Public: get ramfs superblock ---- */
vfs_superblock_t *brights_ramfs_vfs_sb(void)
{
  ramfs_sb.fs_type_name = "ramfs";
  ramfs_sb.sb_ops = &ramfs_sb_ops;
  ramfs_sb.default_fops = &ramfs_file_ops;
  ramfs_sb.fs_private = 0;
  ramfs_sb.active = 1;
  return &ramfs_sb;
}

/* ---- Init VFS inodes table ---- */
void brights_ramfs_vfs_init(void)
{
  for (int i = 0; i < RAMFS_VFS_MAX_INODES; ++i) {
    ramfs_vfs_inodes[i].ino = 0;
    ramfs_vfs_inodes[i].mode = 0;
    ramfs_vfs_inodes[i].size = 0;
    ramfs_vfs_inodes[i].sb = 0;
    ramfs_vfs_inodes[i].fs_private = 0;
  }
}
