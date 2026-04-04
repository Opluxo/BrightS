#ifndef BRIGHTS_DEVFS_H
#define BRIGHTS_DEVFS_H
int brights_devfs_init(void);
#endif

#ifndef BRIGHTS_DEVFS_H
#define BRIGHTS_DEVFS_H
int brights_devfs_init(void);
#endif

#include "devfs.h"

typedef struct {
  const char *name;
  int major;
  int minor;
} brights_devfs_node_t;

static brights_devfs_node_t devfs_nodes[] = {
  {"tty0", 4, 0},
  {"disk0", 8, 0},
};

int brights_devfs_init(void)
{
  return (int)(sizeof(devfs_nodes) / sizeof(devfs_nodes[0]));
}
