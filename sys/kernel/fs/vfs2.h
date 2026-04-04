#ifndef BRIGHTS_VFS2_H
#define BRIGHTS_VFS2_H

#include <stdint.h>

/* File type bits for vfs_inode.mode */
#define VFS_S_IFMT   0xF000
#define VFS_S_IFREG  0x8000
#define VFS_S_IFDIR  0x4000
#define VFS_S_IFCHR  0x2000
#define VFS_S_IFBLK  0x6000
#define VFS_S_IFLNK  0xA000  /* Symbolic link */

/* Permission bits */
#define VFS_S_IRWXU  0x01C0  /* Owner: rwx */
#define VFS_S_IRUSR  0x0100  /* Owner: r */
#define VFS_S_IWUSR  0x0080  /* Owner: w */
#define VFS_S_IXUSR  0x0040  /* Owner: x */
#define VFS_S_IRWXG  0x0038  /* Group: rwx */
#define VFS_S_IRGRP  0x0020  /* Group: r */
#define VFS_S_IWGRP  0x0010  /* Group: w */
#define VFS_S_IXGRP  0x0008  /* Group: x */
#define VFS_S_IRWXO  0x0007  /* Other: rwx */
#define VFS_S_IROTH  0x0004  /* Other: r */
#define VFS_S_IWOTH  0x0002  /* Other: w */
#define VFS_S_IXOTH  0x0001  /* Other: x */

/* Default permissions */
#define VFS_DEFAULT_DIR_MODE  (VFS_S_IFDIR | VFS_S_IRWXU | VFS_S_IRGRP | VFS_S_IXGRP | VFS_S_IROTH | VFS_S_IXOTH)
#define VFS_DEFAULT_FILE_MODE (VFS_S_IFREG | VFS_S_IRUSR | VFS_S_IWUSR | VFS_S_IRGRP | VFS_S_IROTH)

/* Open flags */
#define VFS_O_RDONLY  0x00
#define VFS_O_WRONLY  0x01
#define VFS_O_RDWR    0x02
#define VFS_O_CREAT   0x40
#define VFS_O_TRUNC   0x200
#define VFS_O_APPEND  0x400

/* lseek whence */
#define VFS_SEEK_SET 0
#define VFS_SEEK_CUR 1
#define VFS_SEEK_END 2

/* Max per-process file descriptors */
#define VFS_MAX_FDS 32

/* Max mounts */
#define VFS_MAX_MOUNTS 8

/* Max symlink follow depth */
#define VFS_MAX_SYMLINK_DEPTH 8

/* Forward declarations */
struct vfs_inode;
struct vfs_file;
struct vfs_superblock;
struct vfs_mount;

/* File operations - dispatch table for open files */
typedef struct {
  int64_t (*read)(struct vfs_file *f, void *buf, uint64_t count);
  int64_t (*write)(struct vfs_file *f, const void *buf, uint64_t count);
  int     (*close)(struct vfs_file *f);
  int64_t (*lseek)(struct vfs_file *f, int64_t offset, int whence);
  int     (*ioctl)(struct vfs_file *f, uint64_t request, void *argp);
  int     (*readdir)(struct vfs_file *f, char *name_buf, uint64_t buf_size);
} vfs_file_ops_t;

/* Superblock operations */
typedef struct {
  struct vfs_inode *(*lookup)(struct vfs_superblock *sb, const char *path);
  int     (*create)(struct vfs_superblock *sb, const char *path);
  int     (*mkdir)(struct vfs_superblock *sb, const char *path);
  int     (*unlink)(struct vfs_superblock *sb, const char *path);
  int     (*stat)(struct vfs_superblock *sb, const char *path,
                   uint64_t *size_out, uint32_t *mode_out);
  int     (*readdir)(struct vfs_superblock *sb, const char *dir_path,
                      char *name_buf, uint64_t buf_size);
  int     (*symlink)(struct vfs_superblock *sb, const char *target, const char *linkpath);
  int64_t (*readlink)(struct vfs_superblock *sb, const char *path, char *buf, uint64_t bufsize);
  int     (*chmod)(struct vfs_superblock *sb, const char *path, uint32_t mode);
  int     (*chown)(struct vfs_superblock *sb, const char *path, uint32_t uid, uint32_t gid);
} vfs_sb_ops_t;

/* Inode - represents a file/directory in a filesystem */
typedef struct vfs_inode {
  uint32_t ino;
  uint32_t mode;      /* VFS_S_IFREG, VFS_S_IFDIR, etc. + permissions */
  uint64_t size;
  uint32_t uid;       /* Owner user ID */
  uint32_t gid;       /* Owner group ID */
  uint32_t nlink;     /* Hard link count */
  uint64_t atime;     /* Access time */
  uint64_t mtime;     /* Modification time */
  uint64_t ctime;     /* Change time */
  struct vfs_superblock *sb;
  void *fs_private;   /* filesystem-specific data */
} vfs_inode_t;

/* Superblock - represents a mounted filesystem */
typedef struct vfs_superblock {
  const char *fs_type_name;
  vfs_sb_ops_t *sb_ops;
  vfs_file_ops_t *default_fops;  /* default file operations for this FS */
  void *fs_private;               /* filesystem-specific data */
  int active;
} vfs_superblock_t;

/* File - represents an open file descriptor */
typedef struct vfs_file {
  vfs_inode_t *inode;
  uint64_t pos;
  uint32_t flags;
  vfs_file_ops_t *fops;
  void *private_data;   /* per-open instance data */
  int in_use;
} vfs_file_t;

/* Mount - maps a path prefix to a superblock */
typedef struct vfs_mount {
  char prefix[128];
  vfs_superblock_t *sb;
  int active;
} vfs_mount_t;

/* ----- VFS core API ----- */

/* Initialize VFS subsystem */
void brights_vfs2_init(void);

/* Mount a filesystem at a path prefix */
int brights_vfs2_mount(const char *prefix, vfs_superblock_t *sb);

/* Unmount a filesystem */
int brights_vfs2_umount(const char *prefix);

/* Path-based file operations (resolve path -> FS -> dispatch) */
int     brights_vfs2_open(const char *path, uint32_t flags, vfs_file_t **out);
int     brights_vfs2_close(vfs_file_t *f);
int64_t brights_vfs2_read(vfs_file_t *f, void *buf, uint64_t count);
int64_t brights_vfs2_write(vfs_file_t *f, const void *buf, uint64_t count);
int64_t brights_vfs2_lseek(vfs_file_t *f, int64_t offset, int whence);
int     brights_vfs2_create(const char *path);
int     brights_vfs2_mkdir(const char *path);
int     brights_vfs2_unlink(const char *path);
int     brights_vfs2_stat(const char *path, uint64_t *size_out, uint32_t *mode_out);
int     brights_vfs2_readdir(const char *dir_path, char *name_buf, uint64_t buf_size);

/* Symlink operations */
int     brights_vfs2_symlink(const char *target, const char *linkpath);
int64_t brights_vfs2_readlink(const char *path, char *buf, uint64_t bufsize);

/* Permission operations */
int     brights_vfs2_chmod(const char *path, uint32_t mode);
int     brights_vfs2_chown(const char *path, uint32_t uid, uint32_t gid);

/* Allocate/deallocate a vfs_file_t slot */
vfs_file_t *brights_vfs2_file_alloc(void);
void brights_vfs2_file_free(vfs_file_t *f);

/* Resolve path to mount point and relative path */
vfs_mount_t *brights_vfs2_resolve(const char *full_path, const char **rel_path_out);

/* Per-process fd table management */
void brights_vfs2_fdtable_init(struct vfs_file **table);
int  brights_vfs2_fd_alloc(struct vfs_file **table, vfs_file_t *f);
void brights_vfs2_fd_free(struct vfs_file **table, int fd);
vfs_file_t *brights_vfs2_fd_get(struct vfs_file **table, int fd);

#endif
