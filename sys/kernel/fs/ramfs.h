#ifndef BRIGHTS_RAMFS_H
#define BRIGHTS_RAMFS_H

#include <stdint.h>

#define BRIGHTS_RAMFS_MAX_FILES 32
#define BRIGHTS_RAMFS_MAX_NAME  128
#define BRIGHTS_RAMFS_MAX_SYMLINK_TARGET 256

typedef struct {
  char path[BRIGHTS_RAMFS_MAX_NAME];
  uint64_t size;
  uint32_t mode;      /* File type + permissions */
  uint32_t uid;       /* Owner */
  uint32_t gid;       /* Group */
  int is_dir;
  int is_symlink;
} brights_ramfs_stat_t;

typedef struct {
  char name[BRIGHTS_RAMFS_MAX_NAME];
  uint8_t *data;
  uint64_t size;
  uint64_t capacity;
  uint32_t mode;      /* File type + permissions */
  uint32_t uid;       /* Owner user ID */
  uint32_t gid;       /* Owner group ID */
  char symlink_target[BRIGHTS_RAMFS_MAX_SYMLINK_TARGET]; /* For symlinks */
  int is_dir;
  int is_symlink;
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

/* Symlink support */
int brights_ramfs_symlink(const char *target, const char *linkpath);
int64_t brights_ramfs_readlink(const char *path, char *buf, uint64_t bufsize);

/* Hard link support */
int brights_ramfs_link(const char *oldpath, const char *newpath);

/* Permission support */
int brights_ramfs_chmod(const char *path, uint32_t mode);
int brights_ramfs_chown(const char *path, uint32_t uid, uint32_t gid);

#endif
