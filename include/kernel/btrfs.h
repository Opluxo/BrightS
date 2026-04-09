#ifndef BRIGHTS_BTRFS_H
#define BRIGHTS_BTRFS_H

#include <stdint.h>

int brights_btrfs_probe(void);
int brights_btrfs_mount(void);
int brights_btrfs_list_root(void);
int brights_btrfs_read_by_name(const char *name);
int brights_btrfs_write_by_name(const char *name, const void *data, uint64_t len);

/* Write support */
int brights_btrfs_create(const char *name);
int brights_btrfs_write_file(const char *name, const void *data, uint64_t len);

#endif
