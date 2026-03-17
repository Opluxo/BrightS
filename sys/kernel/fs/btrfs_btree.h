#ifndef BRIGHTS_BTRFS_BTREE_H
#define BRIGHTS_BTRFS_BTREE_H

#include <stdint.h>

int brights_btrfs_btree_insert_leaf(uint64_t bytenr, const void *key, const void *item, uint32_t item_size);

#endif
