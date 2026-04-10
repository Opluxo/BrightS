#ifndef BRIGHTS_BLOCK_H
#define BRIGHTS_BLOCK_H

#include <stdint.h>

#define BRIGHTS_BLOCK_SIZE 4096u
#define BRIGHTS_MAX_BLOCK_DEVS 8

/* Block device type */
typedef enum {
  BRIGHTS_BLOCK_DEV_NONE = 0,
  BRIGHTS_BLOCK_DEV_NVME = 1,
  BRIGHTS_BLOCK_DEV_AHCI = 2,
  BRIGHTS_BLOCK_DEV_RAMDISK = 3,
} brights_block_dev_type_t;

/* Block device structure */
typedef struct {
  int (*read)(uint64_t lba, void *buf, uint64_t count);
  int (*write)(uint64_t lba, const void *buf, uint64_t count);
  const char *name;
  brights_block_dev_type_t type;
  uint64_t total_blocks;    /* Total number of blocks */
  uint32_t block_size;      /* Block size in bytes (usually 4096) */
  int ready;                /* 1 if device is initialized */
} brights_block_dev_t;

/* Set/get the root block device */
void brights_block_set_root(brights_block_dev_t *dev);
brights_block_dev_t *brights_block_root(void);

/* Register a block device and return its index */
int brights_block_register(brights_block_dev_t *dev);

/* Get a block device by index */
brights_block_dev_t *brights_block_get(int index);

/* Get number of registered block devices */
int brights_block_count(void);

/* Read/write from/to the root block device */
int brights_disk_read(uint64_t lba, void *buf, uint64_t nblocks);
int brights_disk_write(uint64_t lba, const void *buf, uint64_t nblocks);

#endif
