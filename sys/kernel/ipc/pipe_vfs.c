#ifndef BRIGHTS_PIPE_VFS_H
#define BRIGHTS_PIPE_VFS_H

#include "vfs2.h"

#define BRIGHTS_PIPE_BUF_SIZE 1024u
#define BRIGHTS_MAX_PIPES 16

typedef struct {
  uint8_t buf[BRIGHTS_PIPE_BUF_SIZE];
  uint32_t rd;
  uint32_t wr;
  uint32_t len;
  int in_use;
} brights_pipe_t;

/* Initialize pipe subsystem */
void brights_pipe_vfs_init(void);

/* Create a pipe, returns two vfs_file_t* in out[0](read) and out[1](write) */
int brights_pipe_vfs_create(vfs_file_t **out_read, vfs_file_t **out_write);

#endif
#include "pipe_vfs.h"
#include "../fs/vfs2.h"
#include <stdint.h>

static brights_pipe_t pipe_pool[BRIGHTS_MAX_PIPES];
static vfs_inode_t pipe_inodes[BRIGHTS_MAX_PIPES * 2]; /* read + write ends */
static vfs_superblock_t pipe_sb;

/* ---- file_ops for pipe read end ---- */
static int64_t pipe_read(vfs_file_t *f, void *buf, uint64_t count)
{
  if (!f || !f->private_data) return -1;
  brights_pipe_t *p = (brights_pipe_t *)f->private_data;
  uint8_t *dst = (uint8_t *)buf;
  uint64_t got = 0;
  while (got < count && p->len > 0) {
    dst[got++] = p->buf[p->rd];
    p->rd = (p->rd + 1u) % BRIGHTS_PIPE_BUF_SIZE;
    --p->len;
  }
  return (int64_t)got;
}

static int64_t pipe_write(vfs_file_t *f, const void *buf, uint64_t count)
{
  (void)f; (void)buf; (void)count;
  return -1; /* Should not write to read end */
}

static int pipe_read_close(vfs_file_t *f)
{
  (void)f;
  return 0;
}

static vfs_file_ops_t pipe_read_fops = {
  .read = pipe_read,
  .write = pipe_write,
  .close = pipe_read_close,
  .lseek = 0,
  .ioctl = 0,
  .readdir = 0,
};

/* ---- file_ops for pipe write end ---- */
static int64_t pipe_wread(vfs_file_t *f, void *buf, uint64_t count)
{
  (void)f; (void)buf; (void)count;
  return -1; /* Should not read from write end */
}

static int64_t pipe_write_data(vfs_file_t *f, const void *buf, uint64_t count)
{
  if (!f || !f->private_data) return -1;
  brights_pipe_t *p = (brights_pipe_t *)f->private_data;
  const uint8_t *src = (const uint8_t *)buf;
  uint64_t wrote = 0;
  while (wrote < count && p->len < BRIGHTS_PIPE_BUF_SIZE) {
    p->buf[p->wr] = src[wrote++];
    p->wr = (p->wr + 1u) % BRIGHTS_PIPE_BUF_SIZE;
    ++p->len;
  }
  return (int64_t)wrote;
}

static int pipe_write_close(vfs_file_t *f)
{
  (void)f;
  return 0;
}

static vfs_file_ops_t pipe_write_fops = {
  .read = pipe_wread,
  .write = pipe_write_data,
  .close = pipe_write_close,
  .lseek = 0,
  .ioctl = 0,
  .readdir = 0,
};

/* ---- Init ---- */
void brights_pipe_vfs_init(void)
{
  for (int i = 0; i < BRIGHTS_MAX_PIPES; ++i) {
    pipe_pool[i].rd = 0;
    pipe_pool[i].wr = 0;
    pipe_pool[i].len = 0;
    pipe_pool[i].in_use = 0;
  }
  pipe_sb.fs_type_name = "pipefs";
  pipe_sb.sb_ops = 0;
  pipe_sb.default_fops = 0;
  pipe_sb.fs_private = 0;
  pipe_sb.active = 1;
}

/* ---- Create a pipe ---- */
int brights_pipe_vfs_create(vfs_file_t **out_read, vfs_file_t **out_write)
{
  if (!out_read || !out_write) return -1;

  /* Find free pipe slot */
  int pi = -1;
  for (int i = 0; i < BRIGHTS_MAX_PIPES; ++i) {
    if (!pipe_pool[i].in_use) {
      pi = i;
      break;
    }
  }
  if (pi < 0) return -1;

  pipe_pool[pi].rd = 0;
  pipe_pool[pi].wr = 0;
  pipe_pool[pi].len = 0;
  pipe_pool[pi].in_use = 1;

  /* Allocate read end */
  vfs_file_t *fr = brights_vfs2_file_alloc();
  vfs_file_t *fw = brights_vfs2_file_alloc();
  if (!fr || !fw) {
    if (fr) brights_vfs2_file_free(fr);
    if (fw) brights_vfs2_file_free(fw);
    pipe_pool[pi].in_use = 0;
    return -1;
  }

  /* Read end */
  fr->inode = &pipe_inodes[pi * 2];
  fr->inode->ino = (uint32_t)(pi * 2);
  fr->inode->mode = VFS_S_IFREG;
  fr->inode->size = 0;
  fr->inode->sb = &pipe_sb;
  fr->inode->fs_private = &pipe_pool[pi];
  fr->pos = 0;
  fr->flags = VFS_O_RDONLY;
  fr->fops = &pipe_read_fops;
  fr->private_data = &pipe_pool[pi];

  /* Write end */
  fw->inode = &pipe_inodes[pi * 2 + 1];
  fw->inode->ino = (uint32_t)(pi * 2 + 1);
  fw->inode->mode = VFS_S_IFREG;
  fw->inode->size = 0;
  fw->inode->sb = &pipe_sb;
  fw->inode->fs_private = &pipe_pool[pi];
  fw->pos = 0;
  fw->flags = VFS_O_WRONLY;
  fw->fops = &pipe_write_fops;
  fw->private_data = &pipe_pool[pi];

  *out_read = fr;
  *out_write = fw;
  return 0;
}
