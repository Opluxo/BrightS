#ifndef BRIGHTS_PIPE_VFS_H
#define BRIGHTS_PIPE_VFS_H
#include "vfs2.h"
vfs_superblock_t *brights_pipe_vfs_sb(void);
void brights_pipe_vfs_init(void);
int brights_pipe_vfs_create(vfs_file_t **out_read, vfs_file_t **out_write);
#endif
