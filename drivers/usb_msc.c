#include "usb.h"
#ifdef __i386__
#include "../arch/i386/io.h"
#else
#include "../arch/x86_64/io.h"
#endif
#include "../include/kernel/kmalloc.h"
#include "../kernel/core/printf.h"
#include "serial.h"
#include "block.h"

/* USB Mass Storage Bulk-Only Transport */
#define MSC_CBW_SIGNATURE  0x43425355
#define MSC_CSW_SIGNATURE  0x53425355

/* Command Block Wrapper */
typedef struct {
  uint32_t dCBWSignature;
  uint32_t dCBWTag;
  uint32_t dCBWDataTransferLength;
  uint8_t  bmCBWFlags;
  uint8_t  bCBWLUN;
  uint8_t  bCBWCBLength;
  uint8_t  CBWCB[16];
} __attribute__((packed)) msc_cbw_t;

/* Command Status Wrapper */
typedef struct {
  uint32_t dCSWSignature;
  uint32_t dCSWTag;
  uint32_t dCSWDataResidue;
  uint8_t  bCSWStatus;
} __attribute__((packed)) msc_csw_t;

/* SCSI commands */
#define SCSI_CMD_TEST_UNIT_READY   0x00
#define SCSI_CMD_REQUEST_SENSE     0x03
#define SCSI_CMD_INQUIRY           0x12
#define SCSI_CMD_READ_CAPACITY_10  0x25
#define SCSI_CMD_READ_10           0x28
#define SCSI_CMD_WRITE_10          0x2A

/* SCSI Read Capacity 10 response */
typedef struct {
  uint32_t lba;
  uint32_t block_size;
} __attribute__((packed)) scsi_read_capacity_10_t;

static usb_controller_t *msc_ctrl = 0;
static usb_device_t     *msc_dev  = 0;
static uint8_t           msc_bulk_in_ep = 0;
static uint8_t           msc_bulk_out_ep = 0;
static uint32_t          msc_tag = 0;
static uint32_t          msc_block_size = 512;
static uint64_t          msc_total_blocks = 0;
static int               msc_initialized = 0;

static int msc_send_cbw(usb_controller_t *ctrl, usb_device_t *dev,
  uint8_t lun, uint32_t transfer_len, uint8_t flags, uint8_t cmd_len,
  const uint8_t *cmd)
{
  msc_cbw_t cbw;
  cbw.dCBWSignature = MSC_CBW_SIGNATURE;
  cbw.dCBWTag = ++msc_tag;
  cbw.dCBWDataTransferLength = transfer_len;
  cbw.bmCBWFlags = flags;
  cbw.bCBWLUN = lun;
  cbw.bCBWCBLength = cmd_len;
  for (int i = 0; i < 16; ++i) {
    cbw.CBWCB[i] = (i < cmd_len) ? cmd[i] : 0;
  }

  return ctrl->bulk_transfer(ctrl, dev->address, msc_bulk_out_ep,
    &cbw, sizeof(cbw), 0);
}

static int msc_recv_csw(usb_controller_t *ctrl, usb_device_t *dev,
  msc_csw_t *csw)
{
  return ctrl->bulk_transfer(ctrl, dev->address, msc_bulk_in_ep,
    csw, sizeof(*csw), 1);
}

static int msc_scsi_command(usb_controller_t *ctrl, usb_device_t *dev,
  uint8_t lun, uint8_t *cmd, uint8_t cmd_len,
  void *data, uint32_t data_len, int dir_in)
{
  uint8_t flags = dir_in ? 0x80 : 0x00;

  if (msc_send_cbw(ctrl, dev, lun, data_len, flags, cmd_len, cmd) != 0) {
    return -1;
  }

  if (data_len > 0 && data) {
    int ret = ctrl->bulk_transfer(ctrl, dev->address,
      dir_in ? msc_bulk_in_ep : msc_bulk_out_ep,
      data, data_len, dir_in);
    if (ret < 0) return -1;
  }

  msc_csw_t csw;
  if (msc_recv_csw(ctrl, dev, &csw) != 0) return -1;

  if (csw.dCSWSignature != MSC_CSW_SIGNATURE) return -1;
  if (csw.bCSWStatus != 0) return -1;

  return 0;
}

static int msc_test_unit_ready(usb_controller_t *ctrl, usb_device_t *dev, uint8_t lun)
{
  uint8_t cmd[6] = {SCSI_CMD_TEST_UNIT_READY, 0, 0, 0, 0, 0};
  return msc_scsi_command(ctrl, dev, lun, cmd, 6, 0, 0, 1);
}

static int msc_read_capacity(usb_controller_t *ctrl, usb_device_t *dev, uint8_t lun)
{
  uint8_t cmd[10] = {SCSI_CMD_READ_CAPACITY_10, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  scsi_read_capacity_10_t resp;

  if (msc_scsi_command(ctrl, dev, lun, cmd, 10, &resp, sizeof(resp), 1) != 0) {
    return -1;
  }

  msc_total_blocks = resp.lba + 1;
  msc_block_size = resp.block_size;
  return 0;
}

static int msc_read_10(usb_controller_t *ctrl, usb_device_t *dev,
  uint8_t lun, uint32_t lba, void *buf, uint16_t count)
{
  uint8_t cmd[10] = {
    SCSI_CMD_READ_10, 0,
    (uint8_t)(lba >> 24), (uint8_t)(lba >> 16),
    (uint8_t)(lba >> 8),  (uint8_t)(lba),
    0, 0,
    (uint8_t)(count >> 8), (uint8_t)(count)
  };

  uint32_t byte_count = count * msc_block_size;
  return msc_scsi_command(ctrl, dev, lun, cmd, 10, buf, byte_count, 1);
}

static int msc_write_10(usb_controller_t *ctrl, usb_device_t *dev,
  uint8_t lun, uint32_t lba, const void *buf, uint16_t count)
{
  uint8_t cmd[10] = {
    SCSI_CMD_WRITE_10, 0,
    (uint8_t)(lba >> 24), (uint8_t)(lba >> 16),
    (uint8_t)(lba >> 8),  (uint8_t)(lba),
    0, 0,
    (uint8_t)(count >> 8), (uint8_t)(count)
  };

  uint32_t byte_count = count * msc_block_size;
  return msc_scsi_command(ctrl, dev, lun, cmd, 10, (void *)buf, byte_count, 0);
}

int brights_usb_msc_init(usb_controller_t *ctrl, usb_device_t *dev)
{
  msc_ctrl = ctrl;
  msc_dev = dev;

  for (int i = 0; i < dev->ep_count; ++i) {
    if (dev->ep_type[i] == USB_EP_BULK) {
      if (dev->ep_in_addr[i] & 0x80) {
        msc_bulk_in_ep = dev->ep_in_addr[i];
      } else {
        msc_bulk_out_ep = dev->ep_in_addr[i];
      }
    }
  }

  if (msc_bulk_in_ep == 0 || msc_bulk_out_ep == 0) return -1;

  /* Wait for device to be ready */
  for (int i = 0; i < 100; ++i) {
    if (msc_test_unit_ready(ctrl, dev, 0) == 0) break;
    for (volatile int j = 0; j < 10000; ++j) __asm__ __volatile__("pause");
  }

  if (msc_read_capacity(ctrl, dev, 0) != 0) return -1;

  msc_initialized = 1;
  return 0;
}

/* Block device wrapper (BRIGHTS_BLOCK_SIZE blocks to SCSI sectors) */
static int usb_msc_block_read(uint64_t block_lba, void *buf, uint64_t count)
{
  if (!msc_initialized) return -1;
  uint32_t sec_per_blk = BRIGHTS_BLOCK_SIZE / msc_block_size;
  uint64_t sector_lba = block_lba * sec_per_blk;
  uint8_t *tmp = (uint8_t *)buf;

  for (uint64_t i = 0; i < count; ++i) {
    if (msc_read_10(msc_ctrl, msc_dev, 0,
      (uint32_t)(sector_lba + i * sec_per_blk),
      tmp + i * BRIGHTS_BLOCK_SIZE, sec_per_blk) != 0) {
      return -1;
    }
  }
  return (int)count;
}

static int usb_msc_block_write(uint64_t block_lba, const void *buf, uint64_t count)
{
  if (!msc_initialized) return -1;
  uint32_t sec_per_blk = BRIGHTS_BLOCK_SIZE / msc_block_size;
  uint64_t sector_lba = block_lba * sec_per_blk;
  const uint8_t *tmp = (const uint8_t *)buf;

  for (uint64_t i = 0; i < count; ++i) {
    if (msc_write_10(msc_ctrl, msc_dev, 0,
      (uint32_t)(sector_lba + i * sec_per_blk),
      tmp + i * BRIGHTS_BLOCK_SIZE, sec_per_blk) != 0) {
      return -1;
    }
  }
  return (int)count;
}

static brights_block_dev_t msc_block_dev = {0};

int brights_usb_msc_register_block(void)
{
  if (!msc_initialized) return -1;
  msc_block_dev.read = usb_msc_block_read;
  msc_block_dev.write = usb_msc_block_write;
  msc_block_dev.name = "usbmsc";
  msc_block_dev.type = BRIGHTS_BLOCK_DEV_USB;
  uint32_t sec_per_blk = BRIGHTS_BLOCK_SIZE / msc_block_size;
  msc_block_dev.total_blocks = msc_total_blocks / sec_per_blk;
  msc_block_dev.block_size = BRIGHTS_BLOCK_SIZE;
  msc_block_dev.ready = 1;
  brights_block_register(&msc_block_dev);
  brights_block_set_root(&msc_block_dev);
  return 0;
}

int brights_usb_msc_read(uint64_t lba, void *buf, uint32_t count)
{
  if (!msc_initialized || !msc_ctrl || !msc_dev) return -1;

  uint32_t remaining = count;
  uint32_t current_lba = (uint32_t)lba;
  uint8_t *ptr = (uint8_t *)buf;

  while (remaining > 0) {
    uint16_t chunk = remaining > 256 ? 256 : (uint16_t)remaining;
    if (msc_read_10(msc_ctrl, msc_dev, 0, current_lba, ptr, chunk) != 0) {
      return -1;
    }
    current_lba += chunk;
    ptr += chunk * msc_block_size;
    remaining -= chunk;
  }

  return (int)count;
}

int brights_usb_msc_write(uint64_t lba, const void *buf, uint32_t count)
{
  if (!msc_initialized || !msc_ctrl || !msc_dev) return -1;

  uint32_t remaining = count;
  uint32_t current_lba = (uint32_t)lba;
  const uint8_t *ptr = (const uint8_t *)buf;

  while (remaining > 0) {
    uint16_t chunk = remaining > 256 ? 256 : (uint16_t)remaining;
    if (msc_write_10(msc_ctrl, msc_dev, 0, current_lba, ptr, chunk) != 0) {
      return -1;
    }
    current_lba += chunk;
    ptr += chunk * msc_block_size;
    remaining -= chunk;
  }

  return (int)count;
}
