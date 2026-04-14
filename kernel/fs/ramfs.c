#include "ramfs.h"
#include "kernel_util.h"
#include <stdint.h>

static brights_ramfs_file_t ramfs_files[BRIGHTS_RAMFS_MAX_FILES];
static uint8_t ramfs_storage[BRIGHTS_RAMFS_MAX_FILES][4096];

static int path_len_limited(const char *s, int max)
{
  int n = 0;
  while (s[n]) {
    if (n >= max) {
      return -1;
    }
    ++n;
  }
  return n;
}

static int path_eq(const char *a, const char *b)
{
  while (*a && *b && *a == *b) {
    ++a;
    ++b;
  }
  return *a == 0 && *b == 0;
}

static int normalize_path(const char *path, char *out, int cap)
{
  if (!path || !out || cap < 2) {
    return -1;
  }

  char segs[16][BRIGHTS_RAMFS_MAX_NAME];
  int seg_count = 0;
  int i = 0;

  while (path[i]) {
    while (path[i] == '/') {
      ++i;
    }
    if (!path[i]) {
      break;
    }

    char seg[BRIGHTS_RAMFS_MAX_NAME];
    int slen = 0;
    while (path[i] && path[i] != '/') {
      if (slen >= BRIGHTS_RAMFS_MAX_NAME - 1) {
        return -1;
      }
      seg[slen++] = path[i++];
    }
    seg[slen] = 0;

    if (slen == 1 && seg[0] == '.') {
      continue;
    }
    if (slen == 2 && seg[0] == '.' && seg[1] == '.') {
      if (seg_count > 0) {
        --seg_count;
      }
      continue;
    }
    if (seg_count >= 16) {
      return -1;
    }
    for (int j = 0; j <= slen; ++j) {
      segs[seg_count][j] = seg[j];
    }
    ++seg_count;
  }

  if (seg_count == 0) {
    out[0] = '/';
    out[1] = 0;
    return 0;
  }

  int p = 0;
  out[p++] = '/';
  for (int s = 0; s < seg_count; ++s) {
    for (int j = 0; segs[s][j]; ++j) {
      if (p >= cap - 1) {
        return -1;
      }
      out[p++] = segs[s][j];
    }
    if (s + 1 < seg_count) {
      if (p >= cap - 1) {
        return -1;
      }
      out[p++] = '/';
    }
  }
  out[p] = 0;
  return 0;
}

static int path_parent(const char *path, char *out, int cap)
{
  if (!path || !out || cap < 2) {
    return -1;
  }
  if (path[0] != '/' || path[1] == 0) {
    out[0] = '/';
    out[1] = 0;
    return 0;
  }

  int last = -1;
  for (int i = 0; path[i]; ++i) {
    if (path[i] == '/') {
      last = i;
    }
  }
  if (last <= 0) {
    out[0] = '/';
    out[1] = 0;
    return 0;
  }
  if (last >= cap) {
    return -1;
  }
  for (int i = 0; i < last; ++i) {
    out[i] = path[i];
  }
  out[last] = 0;
  return 0;
}
static int ramfs_find_free(void)
{
  for (int i = 0; i < BRIGHTS_RAMFS_MAX_FILES; ++i) {
    if (!ramfs_files[i].in_use) {
      return i;
    }
  }
  return -1;
}

static int ramfs_find_name(const char *name)
{
  for (int i = 0; i < BRIGHTS_RAMFS_MAX_FILES; ++i) {
    if (!ramfs_files[i].in_use) {
      continue;
    }
    if (path_eq(ramfs_files[i].name, name)) {
      return i;
    }
  }
  return -1;
}

static int ramfs_parent_exists(const char *path)
{
  char parent[BRIGHTS_RAMFS_MAX_NAME];
  if (path_parent(path, parent, sizeof(parent)) < 0) {
    return 0;
  }
  if (path_eq(parent, "/")) {
    return 1;
  }
  int idx = ramfs_find_name(parent);
  return (idx >= 0 && ramfs_files[idx].is_dir);
}

static int ramfs_dir_has_children(const char *path)
{
  int plen = path_len_limited(path, BRIGHTS_RAMFS_MAX_NAME - 1);
  if (plen < 0) {
    return 0;
  }
  for (int i = 0; i < BRIGHTS_RAMFS_MAX_FILES; ++i) {
    if (!ramfs_files[i].in_use) {
      continue;
    }
    if (path_eq(ramfs_files[i].name, path)) {
      continue;
    }
    if (path_eq(path, "/")) {
      if (ramfs_files[i].name[0] == '/' && ramfs_files[i].name[1] != 0) {
        return 1;
      }
      continue;
    }
    int match = 1;
    for (int j = 0; j < plen; ++j) {
      if (ramfs_files[i].name[j] != path[j]) {
        match = 0;
        break;
      }
    }
    if (!match) {
      continue;
    }
    if (ramfs_files[i].name[plen] == '/') {
      return 1;
    }
  }
  return 0;
}

void brights_ramfs_init(void)
{
  for (int i = 0; i < BRIGHTS_RAMFS_MAX_FILES; ++i) {
    ramfs_files[i].in_use = 0;
    ramfs_files[i].is_dir = 0;
    ramfs_files[i].is_symlink = 0;
    ramfs_files[i].data = ramfs_storage[i];
    ramfs_files[i].size = 0;
    ramfs_files[i].capacity = sizeof(ramfs_storage[i]);
    ramfs_files[i].name[0] = 0;
    ramfs_files[i].mode = 0;
    ramfs_files[i].uid = 0;
    ramfs_files[i].gid = 0;
    ramfs_files[i].symlink_target[0] = 0;
  }
}

/* Initialize file with default permissions */
static void ramfs_init_file_perms(brights_ramfs_file_t *f, int is_dir)
{
  if (is_dir) {
    f->mode = 0x4000 | 0x01ED; /* S_IFDIR | 0755 */
  } else {
    f->mode = 0x8000 | 0x01A4; /* S_IFREG | 0644 */
  }
  f->uid = 0; /* root */
  f->gid = 0; /* root */
  f->is_symlink = 0;
  f->symlink_target[0] = 0;
}

int brights_ramfs_mkdir(const char *name)
{
  char path[BRIGHTS_RAMFS_MAX_NAME];
  if (normalize_path(name, path, sizeof(path)) < 0 || path_eq(path, "/")) {
    return -1;
  }
  if (!ramfs_parent_exists(path) || ramfs_find_name(path) >= 0) {
    return -1;
  }

  int idx = ramfs_find_free();
  if (idx < 0) {
    return -1;
  }
  brights_ramfs_file_t *f = &ramfs_files[idx];
  int nlen = path_len_limited(path, BRIGHTS_RAMFS_MAX_NAME - 1);
  if (nlen <= 0) {
    return -1;
  }
  for (int n = 0; n < nlen; ++n) {
    f->name[n] = path[n];
  }
  f->name[nlen] = 0;
  f->size = 0;
  f->is_dir = 1;
  f->in_use = 1;
  ramfs_init_file_perms(f, 1);
  return idx;
}

int brights_ramfs_create(const char *name)
{
  char path[BRIGHTS_RAMFS_MAX_NAME];
  if (normalize_path(name, path, sizeof(path)) < 0 || path_eq(path, "/")) {
    return -1;
  }
  if (!ramfs_parent_exists(path) || ramfs_find_name(path) >= 0) {
    return -1;
  }

  int idx = ramfs_find_free();
  if (idx < 0) {
    return -1;
  }
  brights_ramfs_file_t *f = &ramfs_files[idx];
  int nlen = path_len_limited(path, BRIGHTS_RAMFS_MAX_NAME - 1);
  if (nlen <= 0) {
    return -1;
  }
  for (int n = 0; n < nlen; ++n) {
    f->name[n] = path[n];
  }
  f->name[nlen] = 0;
  f->size = 0;
  f->is_dir = 0;
  f->in_use = 1;
  ramfs_init_file_perms(f, 0);
  return idx;
}

int brights_ramfs_open(const char *name)
{
  char path[BRIGHTS_RAMFS_MAX_NAME];
  if (normalize_path(name, path, sizeof(path)) < 0 || path_eq(path, "/")) {
    return -1;
  }
  return ramfs_find_name(path);
}

int brights_ramfs_unlink(const char *name)
{
  char path[BRIGHTS_RAMFS_MAX_NAME];
  if (normalize_path(name, path, sizeof(path)) < 0 || path_eq(path, "/")) {
    return -1;
  }
  int idx = ramfs_find_name(path);
  if (idx < 0) {
    return -1;
  }
  if (ramfs_files[idx].is_dir && ramfs_dir_has_children(path)) {
    return -1;
  }
  ramfs_files[idx].in_use = 0;
  ramfs_files[idx].is_dir = 0;
  ramfs_files[idx].name[0] = 0;
  ramfs_files[idx].size = 0;
  return 0;
}

int brights_ramfs_stat(const char *path_in, brights_ramfs_stat_t *out)
{
  char path[BRIGHTS_RAMFS_MAX_NAME];
  if (!out || normalize_path(path_in, path, sizeof(path)) < 0) {
    return -1;
  }
  if (path_eq(path, "/")) {
    out->path[0] = '/';
    out->path[1] = 0;
    out->size = 0;
    out->mode = 0x4000 | 0x01ED; /* S_IFDIR | 0755 */
    out->uid = 0;
    out->gid = 0;
    out->is_dir = 1;
    out->is_symlink = 0;
    return 0;
  }
  int idx = ramfs_find_name(path);
  if (idx < 0) {
    return -1;
  }
  for (int i = 0; ; ++i) {
    out->path[i] = ramfs_files[idx].name[i];
    if (ramfs_files[idx].name[i] == 0) {
      break;
    }
  }
  out->size = ramfs_files[idx].size;
  out->mode = ramfs_files[idx].mode;
  out->uid = ramfs_files[idx].uid;
  out->gid = ramfs_files[idx].gid;
  out->is_dir = ramfs_files[idx].is_dir;
  out->is_symlink = ramfs_files[idx].is_symlink;
  return 0;
}

int64_t brights_ramfs_read(int fd, uint64_t off, void *buf, uint64_t len)
{
  if (fd < 0 || fd >= BRIGHTS_RAMFS_MAX_FILES) {
    return -1;
  }
  brights_ramfs_file_t *f = &ramfs_files[fd];
  if (!f->in_use || f->is_dir) {
    return -1;
  }
  if (off >= f->size) {
    return 0;
  }
  uint64_t to_read = f->size - off;
  if (to_read > len) {
    to_read = len;
  }
  uint8_t *dst = (uint8_t *)buf;
  const uint8_t *src = f->data + off;
  for (uint64_t i = 0; i < to_read; ++i) {
    dst[i] = src[i];
  }
  return (int64_t)to_read;
}

int64_t brights_ramfs_write(int fd, uint64_t off, const void *buf, uint64_t len)
{
  if (fd < 0 || fd >= BRIGHTS_RAMFS_MAX_FILES) {
    return -1;
  }
  brights_ramfs_file_t *f = &ramfs_files[fd];
  if (!f->in_use || f->is_dir) {
    return -1;
  }
  if (off >= f->capacity) {
    return -1;
  }
  uint64_t to_write = len;
  if (off + to_write > f->capacity) {
    to_write = f->capacity - off;
  }
  uint8_t *dst = f->data + off;
  const uint8_t *src = (const uint8_t *)buf;
  for (uint64_t i = 0; i < to_write; ++i) {
    dst[i] = src[i];
  }
  if (off + to_write > f->size) {
    f->size = off + to_write;
  }
  return (int64_t)to_write;
}

uint64_t brights_ramfs_file_size(int fd)
{
  if (fd < 0 || fd >= BRIGHTS_RAMFS_MAX_FILES) {
    return 0;
  }
  if (!ramfs_files[fd].in_use) {
    return 0;
  }
  return ramfs_files[fd].size;
}

int brights_ramfs_is_dir_fd(int fd)
{
  if (fd < 0 || fd >= BRIGHTS_RAMFS_MAX_FILES) {
    return 0;
  }
  if (!ramfs_files[fd].in_use) {
    return 0;
  }
  return ramfs_files[fd].is_dir;
}

uint64_t brights_ramfs_total_capacity(void)
{
  return (uint64_t)BRIGHTS_RAMFS_MAX_FILES * (uint64_t)sizeof(ramfs_storage[0]);
}

int brights_ramfs_count(void)
{
  int n = 0;
  for (int i = 0; i < BRIGHTS_RAMFS_MAX_FILES; ++i) {
    if (ramfs_files[i].in_use) {
      ++n;
    }
  }
  return n;
}

const char *brights_ramfs_name_at(int idx)
{
  if (idx < 0 || idx >= BRIGHTS_RAMFS_MAX_FILES) {
    return 0;
  }
  if (!ramfs_files[idx].in_use) {
    return 0;
  }
  return ramfs_files[idx].name;
}

uint64_t brights_ramfs_size_at(int idx)
{
  if (idx < 0 || idx >= BRIGHTS_RAMFS_MAX_FILES) {
    return 0;
  }
  if (!ramfs_files[idx].in_use) {
    return 0;
  }
  return ramfs_files[idx].size;
}

/* ===== Symlink support ===== */
int brights_ramfs_symlink(const char *target, const char *linkpath)
{
  char path[BRIGHTS_RAMFS_MAX_NAME];
  if (normalize_path(linkpath, path, sizeof(path)) < 0 || path_eq(path, "/")) {
    return -1;
  }
  if (!ramfs_parent_exists(path) || ramfs_find_name(path) >= 0) {
    return -1;
  }

  int idx = ramfs_find_free();
  if (idx < 0) {
    return -1;
  }
  brights_ramfs_file_t *f = &ramfs_files[idx];
  int nlen = path_len_limited(path, BRIGHTS_RAMFS_MAX_NAME - 1);
  if (nlen <= 0) {
    return -1;
  }
  for (int n = 0; n < nlen; ++n) {
    f->name[n] = path[n];
  }
  f->name[nlen] = 0;

  /* Copy symlink target */
  int tlen = 0;
  while (target[tlen] && tlen < BRIGHTS_RAMFS_MAX_SYMLINK_TARGET - 1) {
    f->symlink_target[tlen] = target[tlen];
    ++tlen;
  }
  f->symlink_target[tlen] = 0;

  f->size = 0;
  f->is_dir = 0;
  f->is_symlink = 1;
  f->mode = 0xA000 | 0x01FF; /* S_IFLNK | 0777 */
  f->in_use = 1;
  return idx;
}

int64_t brights_ramfs_readlink(const char *path_in, char *buf, uint64_t bufsize)
{
  char path[BRIGHTS_RAMFS_MAX_NAME];
  if (!buf || bufsize == 0 || normalize_path(path_in, path, sizeof(path)) < 0) {
    return -1;
  }
  int idx = ramfs_find_name(path);
  if (idx < 0 || !ramfs_files[idx].is_symlink) {
    return -1;
  }
  brights_ramfs_file_t *f = &ramfs_files[idx];
  uint64_t i = 0;
  while (f->symlink_target[i] && i < bufsize - 1) {
    buf[i] = f->symlink_target[i];
    ++i;
  }
  buf[i] = 0;
  return (int64_t)i;
}

/* ===== Permission support ===== */
int brights_ramfs_chmod(const char *path_in, uint32_t mode)
{
  char path[BRIGHTS_RAMFS_MAX_NAME];
  if (normalize_path(path_in, path, sizeof(path)) < 0) {
    return -1;
  }
  if (path_eq(path, "/")) {
    return -1; /* Cannot chmod root */
  }
  int idx = ramfs_find_name(path);
  if (idx < 0) {
    return -1;
  }
  /* Preserve file type bits, update permission bits */
  ramfs_files[idx].mode = (ramfs_files[idx].mode & 0xF000) | (mode & 0x0FFF);
  return 0;
}

int brights_ramfs_chown(const char *path_in, uint32_t uid, uint32_t gid)
{
  char path[BRIGHTS_RAMFS_MAX_NAME];
  if (normalize_path(path_in, path, sizeof(path)) < 0) {
    return -1;
  }
  if (path_eq(path, "/")) {
    return -1; /* Cannot chown root */
  }
  int idx = ramfs_find_name(path);
  if (idx < 0) {
    return -1;
  }
  ramfs_files[idx].uid = uid;
  ramfs_files[idx].gid = gid;
  return 0;
}

int brights_ramfs_close(int fd)
{
  /* RAMFS does not need file descriptors cleanup for basic implementation
     All operations are stateless, file descriptors are just indices
     This function exists for API compatibility */
  return 0;
}

int brights_ramfs_get_dir_entries(const char *path, const char **out_names, int max_entries)
{
  char normal_path[BRIGHTS_RAMFS_MAX_NAME];
  if (normalize_path(path, normal_path, sizeof(normal_path)) < 0) {
    return -1;
  }
  
  int count = 0;
  int path_len = kutil_strlen(normal_path);
  
  for (int i = 0; i < BRIGHTS_RAMFS_MAX_FILES && count < max_entries; i++) {
    if (!ramfs_files[i].in_use) continue;
    
    if (kutil_strncmp(ramfs_files[i].name, normal_path, path_len) == 0) {
      const char *entry_name = &ramfs_files[i].name[path_len];
      
      /* Skip root directory itself */
      if (entry_name[0] == '\0') continue;
      
      /* Skip entries with slashes in remaining path (subdirectory entries) */
      if (kutil_strchr(entry_name, '/') != NULL) continue;
      
      out_names[count++] = entry_name[0] == '/' ? entry_name + 1 : entry_name;
    }
  }
  
  return count;
}
