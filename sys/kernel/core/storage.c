#ifndef BRIGHTS_STORAGE_H
#define BRIGHTS_STORAGE_H
int brights_storage_bootstrap(void);
int brights_storage_mount_system(void);
int brights_storage_rescan_and_mount(void);
const char *brights_storage_backend(void);
#endif

#ifndef BRIGHTS_STORAGE_H
#define BRIGHTS_STORAGE_H
int brights_storage_bootstrap(void);
int brights_storage_mount_system(void);
int brights_storage_rescan_and_mount(void);
const char *brights_storage_backend(void);
#endif

#include "storage.h"
#include "../arch/x86_64/pci.h"
#include "../drivers/nvme.h"
#include "../drivers/ahci.h"
#include "../drivers/ramdisk.h"
#include "../drivers/block.h"
#include "../drivers/serial.h"
#include "fs/vfs.h"
#include <stdint.h>

static uint8_t ramdisk_storage[64 * 4096];
static const char *storage_backend = "unknown";

static void storage_debug(const char *msg)
{
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, msg);
}

const char *brights_storage_backend(void)
{
  return storage_backend;
}

int brights_storage_bootstrap(void)
{
  brights_pci_device_t devs[64];
  int count = brights_pci_scan(devs, 64);
  if (count <= 0) {
    storage_debug("storage: pci scan found no devices\r\n");
  }

  for (int i = 0; i < count; ++i) {
    if (devs[i].class_code == 0x01 && devs[i].subclass == 0x08 && devs[i].prog_if == 0x02) {
      storage_debug("storage: nvme controller found\r\n");
      if (brights_nvme_init(&devs[i]) == 0) {
        storage_backend = "nvme";
        return 0;
      }
      storage_debug("storage: nvme init failed\r\n");
    }
  }

  for (int i = 0; i < count; ++i) {
    if (devs[i].class_code == 0x01 && devs[i].subclass == 0x06 && devs[i].prog_if == 0x01) {
      storage_debug("storage: ahci controller found\r\n");
      if (brights_ahci_init(&devs[i]) == 0) {
        storage_backend = "ahci";
        return 0;
      }
      storage_debug("storage: ahci init failed\r\n");
    }
  }

  brights_ramdisk_init(ramdisk_storage, sizeof(ramdisk_storage));
  brights_block_set_root(brights_ramdisk_dev());
  storage_backend = "ramdisk";
  return 0;
}

int brights_storage_mount_system(void)
{
  return brights_vfs_mount_external(storage_backend);
}

int brights_storage_rescan_and_mount(void)
{
  if (brights_storage_bootstrap() != 0) {
    return -1;
  }
  return brights_storage_mount_system();
}
