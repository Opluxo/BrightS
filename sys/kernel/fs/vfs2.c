#include "vfs2.h"
#include "kmalloc.h"
#include "kernel_util.h"
#include "kernel/stddef.h"

/* ---- Global file pool ---- */
#define VFS_FILE_POOL_SIZE 64

static vfs_file_t file_pool[VFS_FILE_POOL_SIZE];
static uint64_t file_pool_bitmap;  /* Bit 0=free, 1=used */

/* ---- Mount table ---- */
static vfs_mount_t mount_table[VFS_MAX_MOUNTS];
static uint8_t mount_bitmap;  /* Bit 0=free, 1=used */

/* Path resolution cache */
#define PATH_CACHE_SIZE 16
static struct {
    char path[128];
    vfs_mount_t *mount;
    char rel_path[128];
} path_cache[PATH_CACHE_SIZE];
static int path_cache_next = 0;

/* ---- String helpers ---- */
static int vfs_str_len(const char *s)
{
  int n = 0;
  while (s[n]) ++n;
  return n;
}

static int vfs_str_eq(const char *a, const char *b)
{
  while (*a && *b && *a == *b) { ++a; ++b; }
  return *a == 0 && *b == 0;
}

static int vfs_str_prefix(const char *prefix, const char *path)
{
  while (*prefix) {
    if (*prefix != *path) return 0;
    ++prefix; ++path;
  }
  return 1;
}

static int vfs_str_copy(char *dst, int cap, const char *src)
{
  int i = 0;
  while (src[i] && i < cap - 1) {
    dst[i] = src[i];
    ++i;
  }
  dst[i] = 0;
  return i;
}

/* ---- Normalize path: resolve . and .. ---- */
static int vfs_normalize(const char *path, char *out, int cap)
{
  char segs[16][64];
  int seg_count = 0;
  int i = 0;

  while (path[i]) {
    while (path[i] == '/') ++i;
    if (!path[i]) break;

    char seg[64];
    int slen = 0;
    while (path[i] && path[i] != '/') {
      if (slen >= 63) return -1;
      seg[slen++] = path[i++];
    }
    seg[slen] = 0;

    if (slen == 1 && seg[0] == '.') continue;
    if (slen == 2 && seg[0] == '.' && seg[1] == '.') {
      if (seg_count > 0) --seg_count;
      continue;
    }
    if (seg_count >= 16) return -1;
    for (int j = 0; j <= slen; ++j) segs[seg_count][j] = seg[j];
    ++seg_count;
  }

  int p = 0;
  out[p++] = '/';
  for (int s = 0; s < seg_count; ++s) {
    for (int j = 0; segs[s][j]; ++j) {
      if (p >= cap - 1) return -1;
      out[p++] = segs[s][j];
    }
    if (s + 1 < seg_count) {
      if (p >= cap - 1) return -1;
      out[p++] = '/';
    }
  }
  out[p] = 0;
  return 0;
}

/* ---- VFS init ---- */
void brights_vfs2_init(void)
{
  file_pool_bitmap = 0;
  mount_bitmap = 0;
  for (int i = 0; i < VFS_FILE_POOL_SIZE; ++i) {
    file_pool[i].in_use = 0;
  }
  for (int i = 0; i < VFS_MAX_MOUNTS; ++i) {
    mount_table[i].active = 0;
  }
}

/* ---- Mount / unmount ---- */
int brights_vfs2_mount(const char *prefix, vfs_superblock_t *sb)
{
  if (!prefix || !sb) return -1;

  /* Check if already mounted */
  for (int i = 0; i < VFS_MAX_MOUNTS; ++i) {
    if ((mount_bitmap & (1 << i)) && vfs_str_eq(mount_table[i].prefix, prefix)) {
      return -1;
    }
  }

  for (int i = 0; i < VFS_MAX_MOUNTS; ++i) {
    if (!(mount_bitmap & (1 << i))) {
      vfs_str_copy(mount_table[i].prefix, sizeof(mount_table[i].prefix), prefix);
      mount_table[i].sb = sb;
      mount_table[i].active = 1;
      mount_bitmap |= (1 << i);
      return 0;
    }
  }
  return -1;
}

int brights_vfs2_umount(const char *prefix)
{
  if (!prefix) return -1;
  for (int i = 0; i < VFS_MAX_MOUNTS; ++i) {
    if ((mount_bitmap & (1 << i)) && vfs_str_eq(mount_table[i].prefix, prefix)) {
      mount_table[i].active = 0;
      mount_bitmap &= ~(1 << i);
      return 0;
    }
  }
  return -1;
}

/* ---- Path resolution: find longest-prefix mount ---- */
vfs_mount_t *brights_vfs2_resolve(const char *full_path, const char **rel_path_out)
{
  if (!full_path) return 0;

  char norm[256];
  if (vfs_normalize(full_path, norm, sizeof(norm)) < 0) return 0;

  /* Check cache first */
  for (int i = 0; i < PATH_CACHE_SIZE; i++) {
    if (path_cache[i].mount && vfs_str_eq(path_cache[i].path, full_path)) {
      if (rel_path_out) *rel_path_out = path_cache[i].rel_path;
      return path_cache[i].mount;
    }
  }

  vfs_mount_t *best = 0;
  int best_len = 0;
  char best_rel_path[256];

  for (int i = 0; i < VFS_MAX_MOUNTS; ++i) {
    if (!mount_table[i].active) continue;

    const char *mp = mount_table[i].prefix;
    int ml = vfs_str_len(mp);

    /* Root mount "/" matches everything */
    if (ml == 1 && mp[0] == '/') {
      if (best_len < 1) {
        best = &mount_table[i];
        best_len = 1;
        vfs_str_copy(best_rel_path, sizeof(best_rel_path), norm);
      }
      continue;
    }

    if (vfs_str_prefix(mp, norm)) {
      char next = norm[ml];
      if (next == 0 || next == '/') {
        if (ml > best_len) {
          best = &mount_table[i];
          best_len = ml;
          vfs_str_copy(best_rel_path, sizeof(best_rel_path),
                      (next == 0) ? "/" : &norm[ml]);
        }
      }
    }
  }

  if (best) {
    /* Update cache */
    path_cache[path_cache_next].mount = best;
    vfs_str_copy(path_cache[path_cache_next].path, sizeof(path_cache[path_cache_next].path), full_path);
    vfs_str_copy(path_cache[path_cache_next].rel_path, sizeof(path_cache[path_cache_next].rel_path), best_rel_path);
    path_cache_next = (path_cache_next + 1) % PATH_CACHE_SIZE;

    if (rel_path_out) *rel_path_out = best_rel_path;
  }

  return best;
}

/* ---- File pool ---- */
vfs_file_t *brights_vfs2_file_alloc(void)
{
  for (int i = 0; i < VFS_FILE_POOL_SIZE; ++i) {
    if (!(file_pool_bitmap & (1ULL << i))) {
      file_pool_bitmap |= (1ULL << i);
      file_pool[i].in_use = 1;
      file_pool[i].pos = 0;
      file_pool[i].flags = 0;
      file_pool[i].inode = 0;
      file_pool[i].fops = 0;
      file_pool[i].private_data = 0;
      return &file_pool[i];
    }
  }
  return 0;
}

void brights_vfs2_file_free(vfs_file_t *f)
{
  if (!f) return;
  int idx = (int)(f - file_pool);
  if (idx >= 0 && idx < VFS_FILE_POOL_SIZE) {
    file_pool_bitmap &= ~(1ULL << idx);
  }
  f->in_use = 0;
}

/* ---- VFS-level file operations ---- */
int brights_vfs2_open(const char *path, uint32_t flags, vfs_file_t **out)
{
  if (!path || !out) return -1;

  const char *rel = 0;
  vfs_mount_t *mt = brights_vfs2_resolve(path, &rel);
  if (!mt || !mt->sb) return -1;

  vfs_superblock_t *sb = mt->sb;

  /* If O_CREAT, try create first */
  if ((flags & VFS_O_CREAT) && sb->sb_ops && sb->sb_ops->create) {
    sb->sb_ops->create(sb, rel);
  }

  /* Lookup inode */
  vfs_inode_t *inode = 0;
  if (sb->sb_ops && sb->sb_ops->lookup) {
    inode = sb->sb_ops->lookup(sb, rel);
  }
  if (!inode) return -1;

  vfs_file_t *f = brights_vfs2_file_alloc();
  if (!f) return -1;

  f->inode = inode;
  f->pos = 0;
  f->flags = flags;
  f->fops = sb->default_fops;
  f->private_data = inode->fs_private;

  /* O_TRUNC: truncate file */
  if ((flags & VFS_O_TRUNC) && inode->mode == VFS_S_IFREG) {
    inode->size = 0;
  }

  /* O_APPEND: seek to end */
  if (flags & VFS_O_APPEND) {
    f->pos = inode->size;
  }

  *out = f;
  return 0;
}

int brights_vfs2_close(vfs_file_t *f)
{
  if (!f || !f->in_use) return -1;
  if (f->fops && f->fops->close) {
    f->fops->close(f);
  }
  brights_vfs2_file_free(f);
  return 0;
}

int64_t brights_vfs2_read(vfs_file_t *f, void *buf, uint64_t count)
{
  if (!f || !f->in_use || !f->fops || !f->fops->read) return -1;
  return f->fops->read(f, buf, count);
}

int64_t brights_vfs2_write(vfs_file_t *f, const void *buf, uint64_t count)
{
  if (!f || !f->in_use || !f->fops || !f->fops->write) return -1;
  return f->fops->write(f, buf, count);
}

int64_t brights_vfs2_lseek(vfs_file_t *f, int64_t offset, int whence)
{
  if (!f || !f->in_use) return -1;
  if (f->fops && f->fops->lseek) {
    return f->fops->lseek(f, offset, whence);
  }
  /* Default lseek */
  switch (whence) {
    case VFS_SEEK_SET: f->pos = (uint64_t)offset; break;
    case VFS_SEEK_CUR: f->pos = (uint64_t)((int64_t)f->pos + offset); break;
    case VFS_SEEK_END:
      if (f->inode) f->pos = (uint64_t)((int64_t)f->inode->size + offset);
      break;
    default: return -1;
  }
  return (int64_t)f->pos;
}

int brights_vfs2_create(const char *path)
{
  if (!path) return -1;
  const char *rel = 0;
  vfs_mount_t *mt = brights_vfs2_resolve(path, &rel);
  if (!mt || !mt->sb || !mt->sb->sb_ops || !mt->sb->sb_ops->create) return -1;
  return mt->sb->sb_ops->create(mt->sb, rel);
}

int brights_vfs2_mkdir(const char *path)
{
  if (!path) return -1;
  const char *rel = 0;
  vfs_mount_t *mt = brights_vfs2_resolve(path, &rel);
  if (!mt || !mt->sb || !mt->sb->sb_ops || !mt->sb->sb_ops->mkdir) return -1;
  return mt->sb->sb_ops->mkdir(mt->sb, rel);
}

int brights_vfs2_unlink(const char *path)
{
  if (!path) return -1;
  const char *rel = 0;
  vfs_mount_t *mt = brights_vfs2_resolve(path, &rel);
  if (!mt || !mt->sb || !mt->sb->sb_ops || !mt->sb->sb_ops->unlink) return -1;
  return mt->sb->sb_ops->unlink(mt->sb, rel);
}

int brights_vfs2_stat(const char *path, uint64_t *size_out, uint32_t *mode_out)
{
  if (!path) return -1;
  const char *rel = 0;
  vfs_mount_t *mt = brights_vfs2_resolve(path, &rel);
  if (!mt || !mt->sb || !mt->sb->sb_ops || !mt->sb->sb_ops->stat) return -1;
  return mt->sb->sb_ops->stat(mt->sb, rel, size_out, mode_out);
}

int brights_vfs2_readdir(const char *dir_path, char *name_buf, uint64_t buf_size)
{
  if (!dir_path || !name_buf) return -1;
  const char *rel = 0;
  vfs_mount_t *mt = brights_vfs2_resolve(dir_path, &rel);
  if (!mt || !mt->sb || !mt->sb->sb_ops || !mt->sb->sb_ops->readdir) return -1;
  return mt->sb->sb_ops->readdir(mt->sb, rel, name_buf, buf_size);
}

/* ---- Per-process fd table ---- */
void brights_vfs2_fdtable_init(vfs_file_t **table)
{
  for (int i = 0; i < VFS_MAX_FDS; ++i) {
    table[i] = 0;
  }
}

int brights_vfs2_fd_alloc(vfs_file_t **table, vfs_file_t *f)
{
  if (!table || !f) return -1;
  /* fd 0,1,2 are reserved for stdin/stdout/stderr but can be allocated if free */
  for (int i = 0; i < VFS_MAX_FDS; ++i) {
    if (!table[i]) {
      table[i] = f;
      return i;
    }
  }
  return -1;
}

void brights_vfs2_fd_free(vfs_file_t **table, int fd)
{
  if (!table || fd < 0 || fd >= VFS_MAX_FDS) return;
  table[fd] = 0;
}

vfs_file_t *brights_vfs2_fd_get(vfs_file_t **table, int fd)
{
  if (!table || fd < 0 || fd >= VFS_MAX_FDS) return 0;
  return table[fd];
}

/* ---- Symlink operations ---- */
int brights_vfs2_symlink(const char *target, const char *linkpath)
{
  if (!target || !linkpath) return -1;
  const char *rel = 0;
  vfs_mount_t *mt = brights_vfs2_resolve(linkpath, &rel);
  if (!mt || !mt->sb || !mt->sb->sb_ops || !mt->sb->sb_ops->symlink) return -1;
  return mt->sb->sb_ops->symlink(mt->sb, target, rel);
}

int64_t brights_vfs2_readlink(const char *path, char *buf, uint64_t bufsize)
{
  if (!path || !buf) return -1;
  const char *rel = 0;
  vfs_mount_t *mt = brights_vfs2_resolve(path, &rel);
  if (!mt || !mt->sb || !mt->sb->sb_ops || !mt->sb->sb_ops->readlink) return -1;
  return mt->sb->sb_ops->readlink(mt->sb, rel, buf, bufsize);
}

/* ---- Permission operations ---- */
int brights_vfs2_chmod(const char *path, uint32_t mode)
{
  if (!path) return -1;
  const char *rel = 0;
  vfs_mount_t *mt = brights_vfs2_resolve(path, &rel);
  if (!mt || !mt->sb || !mt->sb->sb_ops || !mt->sb->sb_ops->chmod) return -1;
  return mt->sb->sb_ops->chmod(mt->sb, rel, mode);
}

int brights_vfs2_chown(const char *path, uint32_t uid, uint32_t gid)
{
  if (!path) return -1;
  const char *rel = 0;
  vfs_mount_t *mt = brights_vfs2_resolve(path, &rel);
  if (!mt || !mt->sb || !mt->sb->sb_ops || !mt->sb->sb_ops->chown) return -1;
  return mt->sb->sb_ops->chown(mt->sb, rel, uid, gid);
}
