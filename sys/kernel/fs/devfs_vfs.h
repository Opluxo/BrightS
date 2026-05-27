#ifndef BRIGHTS_DEVFS_VFS_H
#define BRIGHTS_DEVFS_VFS_H
#include "fs/vfs2.h"
vfs_superblock_t *brights_devfs_vfs_sb(void);
void brights_devfs_vfs_init(void);
#endif
