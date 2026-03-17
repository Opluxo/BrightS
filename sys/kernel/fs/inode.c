#include "vnode.h"

void brights_inode_from_vnode(const brights_vnode_t *vn, uint64_t *ino_out)
{
  if (!ino_out) {
    return;
  }
  *ino_out = vn ? vn->ino : 0;
}
