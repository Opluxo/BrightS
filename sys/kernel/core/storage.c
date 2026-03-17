#include "storage.h"
#include "../platform/x86_64/pci.h"
#include "../dev/nvme.h"
#include "../dev/ahci.h"
#include "../dev/ramdisk.h"
#include "../dev/block.h"
#include "../fs/vfs.h"
#include <stdint.h>

static uint8_t ramdisk_storage[64 * 4096];
static const char *storage_backend = "unknown";

const char *brights_storage_backend(void)
{
  return storage_backend;
}

int brights_storage_bootstrap(void)
{
  brights_pci_device_t devs[64];
  int count = brights_pci_scan(devs, 64);

  for (int i = 0; i < count; ++i) {
    if (devs[i].class_code == 0x01 && devs[i].subclass == 0x08 && devs[i].prog_if == 0x02) {
      if (brights_nvme_init(&devs[i]) == 0) {
        brights_block_set_root(brights_nvme_block());
        storage_backend = "nvme";
        return 0;
      }
    }
  }

  for (int i = 0; i < count; ++i) {
    if (devs[i].class_code == 0x01 && devs[i].subclass == 0x06 && devs[i].prog_if == 0x01) {
      if (brights_ahci_init(&devs[i]) == 0) {
        brights_block_set_root(brights_ahci_block());
        storage_backend = "ahci";
        return 0;
      }
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
