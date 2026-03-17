#include "ramfs.h"
#include <stdint.h>

static brights_ramfs_file_t ramfs_files[BRIGHTS_RAMFS_MAX_FILES];
static uint8_t ramfs_storage[BRIGHTS_RAMFS_MAX_FILES][4096];

static int name_len_limited(const char *s, int max)
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

static int name_eq(const char *a, const char *b)
{
  while (*a && *b && *a == *b) {
    ++a;
    ++b;
  }
  return *a == 0 && *b == 0;
}

static const char *ramfs_strip_root(const char *path)
{
  if (!path) {
    return "";
  }
  if (path[0] == '/' && path[1] != '\0') {
    return path + 1;
  }
  return path;
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
    if (name_eq(ramfs_files[i].name, name)) {
      return i;
    }
  }
  return -1;
}

void brights_ramfs_init(void)
{
  for (int i = 0; i < BRIGHTS_RAMFS_MAX_FILES; ++i) {
    ramfs_files[i].in_use = 0;
    ramfs_files[i].data = ramfs_storage[i];
    ramfs_files[i].size = 0;
    ramfs_files[i].capacity = sizeof(ramfs_storage[i]);
  }
}

int brights_ramfs_create(const char *name)
{
  name = ramfs_strip_root(name);
  int nlen = name_len_limited(name, BRIGHTS_RAMFS_MAX_NAME - 1);
  if (nlen <= 0) {
    return -1;
  }
  if (ramfs_find_name(name) >= 0) {
    return -1;
  }
  int idx = ramfs_find_free();
  if (idx < 0) {
    return -1;
  }
  brights_ramfs_file_t *f = &ramfs_files[idx];
  for (int n = 0; n < nlen; ++n) {
    f->name[n] = name[n];
  }
  f->name[nlen] = 0;
  f->size = 0;
  f->in_use = 1;
  return idx;
}

int brights_ramfs_open(const char *name)
{
  name = ramfs_strip_root(name);
  return ramfs_find_name(name);
}

int brights_ramfs_unlink(const char *name)
{
  name = ramfs_strip_root(name);
  int idx = ramfs_find_name(name);
  if (idx < 0) {
    return -1;
  }
  ramfs_files[idx].in_use = 0;
  ramfs_files[idx].name[0] = 0;
  ramfs_files[idx].size = 0;
  return 0;
}

int64_t brights_ramfs_read(int fd, uint64_t off, void *buf, uint64_t len)
{
  if (fd < 0 || fd >= BRIGHTS_RAMFS_MAX_FILES) {
    return -1;
  }
  brights_ramfs_file_t *f = &ramfs_files[fd];
  if (!f->in_use) {
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
  if (!f->in_use) {
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
