#ifndef BRIGHTS_VNODE_H
#define BRIGHTS_VNODE_H

#include <stdint.h>

typedef struct {
  uint64_t ino;
  uint32_t mode;
  uint32_t refcnt;
} brights_vnode_t;

void brights_vnode_init(brights_vnode_t *vn, uint64_t ino, uint32_t mode);
void brights_vnode_ref(brights_vnode_t *vn);
void brights_vnode_unref(brights_vnode_t *vn);

#endif
