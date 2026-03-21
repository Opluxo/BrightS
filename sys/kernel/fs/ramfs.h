#ifndef BRIGHTS_RAMFS_H
#define BRIGHTS_RAMFS_H

#include <stdint.h>

#define BRIGHTS_RAMFS_MAX_FILES 32
#define BRIGHTS_RAMFS_MAX_NAME  128

typedef struct {
  char path[BRIGHTS_RAMFS_MAX_NAME];
  uint64_t size;
  int is_dir;
} brights_ramfs_stat_t;

typedef struct {
  char name[BRIGHTS_RAMFS_MAX_NAME];
  uint8_t *data;
  uint64_t size;
  uint64_t capacity;
  int is_dir;
  int in_use;
} brights_ramfs_file_t;

void brights_ramfs_init(void);
int brights_ramfs_mkdir(const char *name);
int brights_ramfs_create(const char *name);
int brights_ramfs_open(const char *name);
int brights_ramfs_unlink(const char *name);
int brights_ramfs_stat(const char *path, brights_ramfs_stat_t *out);
int64_t brights_ramfs_read(int fd, uint64_t off, void *buf, uint64_t len);
int64_t brights_ramfs_write(int fd, uint64_t off, const void *buf, uint64_t len);
uint64_t brights_ramfs_file_size(int fd);
int brights_ramfs_is_dir_fd(int fd);
uint64_t brights_ramfs_total_capacity(void);
int brights_ramfs_count(void);
const char *brights_ramfs_name_at(int idx);
uint64_t brights_ramfs_size_at(int idx);

#endif
