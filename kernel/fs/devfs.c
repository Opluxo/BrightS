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

int brights_devfs_init(void)
{
  return 2; /* tty0 and disk0 */
}
