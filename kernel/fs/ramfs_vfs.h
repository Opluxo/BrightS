#ifndef BRIGHTS_RAMFS_VFS_H
#define BRIGHTS_RAMFS_VFS_H
#include "fs/vfs2.h"
vfs_superblock_t *brights_ramfs_vfs_sb(void);
void brights_ramfs_vfs_init(void);
#endif
