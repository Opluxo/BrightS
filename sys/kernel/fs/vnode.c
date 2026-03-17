#include "vnode.h"

void brights_vnode_init(brights_vnode_t *vn, uint64_t ino, uint32_t mode)
{
  if (!vn) {
    return;
  }
  vn->ino = ino;
  vn->mode = mode;
  vn->refcnt = 1;
}

void brights_vnode_ref(brights_vnode_t *vn)
{
  if (!vn) {
    return;
  }
  ++vn->refcnt;
}

void brights_vnode_unref(brights_vnode_t *vn)
{
  if (!vn || vn->refcnt == 0) {
    return;
  }
  --vn->refcnt;
}
