#include "vnode.h"
#include "ramfs.h"

int brights_namei(const char *path, brights_vnode_t **out)
{
  static brights_vnode_t ramfs_vnode;
  if (!path || !out) {
    return -1;
  }

  int fd = brights_ramfs_open(path);
  if (fd < 0) {
    return -1;
  }

  // Map RAMFS fd directly to inode number for early bootstrap.
  brights_vnode_init(&ramfs_vnode, (uint64_t)(fd + 1), 0);
  *out = &ramfs_vnode;
  return 0;
}
