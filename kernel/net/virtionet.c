#include "virtionet.h"
#include "virtio.h"
#include "net.h"
#include "kernel_util.h"
#include "printf.h"
#include "pmem.h"
#include "serial.h"
#include "pci.h"
#include "io.h"

static brights_console_t virtio_con;
static int virtio_initialized = 0;

/* PCI device info */
static brights_pci_device_t virtio_pci_dev;
static int virtio_pci_found = 0;

/* MAC address from device */
static uint8_t virtio_mac[6];

/* I/O port base (legacy virtio uses I/O BAR) */
static uint16_t virtio_io_base = 0;

/* Virtqueues */
static virtq_t rx_virtq;
static virtq_t tx_virtq;

/* Feature bits we support */
#define VIRTIO_NET_GUEST_FEATURES ((uint32_t)(VIRTIO_NET_F_MAC | VIRTIO_NET_F_STATUS))

/* Receive buffer pool */
#define RX_BUF_COUNT 32
#define RX_BUF_SIZE 2048
static uint8_t rx_bufs[RX_BUF_COUNT][RX_BUF_SIZE] __attribute__((aligned(4096)));
static int rx_next_buf = 0;

/* Transmit buffer pool (DMA-safe, page-aligned) */
#define TX_BUF_COUNT 32
#define TX_BUF_SIZE (VIRTIO_NET_HDR_SIZE + 1514)
static uint8_t tx_bufs[TX_BUF_COUNT][TX_BUF_SIZE] __attribute__((aligned(4096)));
static int tx_next_buf = 0;

/* ---- I/O port helpers ---- */
static inline uint32_t virtio_read32(uint32_t off)
{
  return inl(virtio_io_base + off);
}

static inline void virtio_write32(uint32_t off, uint32_t v)
{
  outl(virtio_io_base + off, v);
}

static inline uint16_t virtio_read16(uint32_t off)
{
  return inw(virtio_io_base + off);
}

static inline void virtio_write16(uint32_t off, uint16_t v)
{
  outw(virtio_io_base + off, v);
}

static inline uint8_t virtio_read8(uint32_t off)
{
  return inb(virtio_io_base + off);
}

static inline void virtio_write8(uint32_t off, uint8_t v)
{
  outb(virtio_io_base + off, v);
}

/* ---- Virtqueue operations ---- */

static int virtq_alloc_descriptor(virtq_t *vq, uint16_t *idx)
{
  if (vq->free_count == 0) return -1;
  --vq->free_count;
  *idx = vq->free_stack[vq->free_count];
  return 0;
}

static void virtq_free_descriptor(virtq_t *vq, uint16_t idx)
{
  vq->free_stack[vq->free_count] = idx;
  ++vq->free_count;
}

static void virtq_add_avail(virtq_t *vq, uint16_t idx)
{
  uint16_t ai = vq->avail->idx;
  vq->avail->ring[ai & (vq->queue_size - 1)] = idx;
  __asm__ __volatile__("" ::: "memory");
  vq->avail->idx = ai + 1;
}

static int virtq_get_used(virtq_t *vq, uint16_t *idx, uint32_t *len)
{
  if (vq->last_used_idx == vq->used->idx) return -1;
  __asm__ __volatile__("" ::: "memory");
  uint16_t ui = vq->last_used_idx & (vq->queue_size - 1);
  *idx = vq->used->ring[ui].id;
  *len = vq->used->ring[ui].len;
  ++vq->last_used_idx;
  return 0;
}

static void virtq_notify(virtq_t *vq)
{
  (void)vq;
  virtio_write16(VIRTIO_LEGACY_QUEUE_NOTIFY, 0);
}

static int virtq_init(virtq_t *vq, int queue_idx, uint16_t queue_size)
{
  kutil_memset(vq, 0, sizeof(*vq));

  /* Select the queue */
  virtio_write16(VIRTIO_LEGACY_QUEUE_SEL, queue_idx);
  uint16_t dev_size = virtio_read16(VIRTIO_LEGACY_QUEUE_SIZE);
  if (dev_size == 0) return -1;
  if (dev_size < queue_size) queue_size = dev_size;

  vq->queue_size = queue_size;

  /* Calculate sizes */
  uint32_t desc_size = queue_size * sizeof(virtq_desc_t);
  uint32_t avail_size = 2 + 2 + queue_size * 2;
  uint32_t used_size = 2 + 2 + queue_size * 8;
  uint32_t total = desc_size + avail_size + used_size;
  uint32_t pages = (total + 4095) / 4096;

  /* Allocate physically contiguous pages */
  void *phys = brights_pmem_alloc_pages(pages);
  if (!phys) return -1;

  uint32_t paddr = (uint32_t)(uintptr_t)phys;
  uint8_t *base = (uint8_t *)phys;

  kutil_memset(base, 0, pages * 4096);

  vq->desc_paddr = paddr;
  vq->avail_paddr = paddr + desc_size;
  vq->used_paddr = paddr + desc_size + avail_size;
  vq->desc = (virtq_desc_t *)base;
  vq->avail = (virtq_avail_t *)(base + desc_size);
  vq->used = (virtq_used_t *)(base + desc_size + avail_size);

  /* Init free stack: desc 0 is head of free chain */
  for (uint16_t i = 0; i < queue_size; ++i) {
    vq->free_stack[i] = i;
  }
  vq->free_count = queue_size;
  vq->last_used_idx = 0;

  /* Tell device the queue physical address */
  virtio_write16(VIRTIO_LEGACY_QUEUE_SEL, queue_idx);
  virtio_write32(VIRTIO_LEGACY_QUEUE_ADDR, paddr);

  return 0;
}

/* ---- PCI detection ---- */
#define VIRTIO_PCI_VENDOR 0x1AF4
#define VIRTIO_PCI_LEGACY_NET 0x1000

static int virtionet_pci_find(void)
{
  brights_pci_device_t devs[8];
  int count = brights_pci_scan(devs, 8);

  for (int i = 0; i < count; ++i) {
    if (devs[i].vendor_id == VIRTIO_PCI_VENDOR &&
        devs[i].device_id == VIRTIO_PCI_LEGACY_NET) {
      virtio_pci_dev = devs[i];
      virtio_pci_found = 1;

      brights_print(&virtio_con, u"virtionet: found PCI ");
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, " ");
      const char *hx = "0123456789ABCDEF";
      char idbuf[5] = {hx[(devs[i].vendor_id >> 12) & 0xF], hx[(devs[i].vendor_id >> 8) & 0xF], hx[(devs[i].vendor_id >> 4) & 0xF], hx[devs[i].vendor_id & 0xF], 0};
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, idbuf);
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, ":");
      char didbuf[5] = {hx[(devs[i].device_id >> 12) & 0xF], hx[(devs[i].device_id >> 8) & 0xF], hx[(devs[i].device_id >> 4) & 0xF], hx[devs[i].device_id & 0xF], 0};
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, didbuf);
      brights_print(&virtio_con, u"\r\n");
      return 0;
    }
  }

  return -1;
}

/* ---- BAR setup ---- */
static int virtionet_map_pci(void)
{
  uint32_t bar0 = virtio_pci_dev.bar[0];

  if (bar0 & 1) {
    /* I/O BAR */
    virtio_io_base = bar0 & 0xFFFC;
    brights_print(&virtio_con, u"virtionet: I/O BAR at 0x");
    const char *hx = "0123456789ABCDEF";
    char iobuf[9];
    for (int j = 0; j < 8; ++j) iobuf[j] = hx[(virtio_io_base >> (28 - j * 4)) & 0xF];
    iobuf[8] = 0;
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, iobuf);
    brights_print(&virtio_con, u"\r\n");
  } else {
    brights_print(&virtio_con, u"virtionet: unsupported MMIO BAR\r\n");
    return -1;
  }

  /* Enable bus master */
  brights_pci_enable_mmio_busmaster(&virtio_pci_dev);

  return 0;
}

/* ---- Feature negotiation ---- */
static int virtionet_negotiate_features(void)
{
  /* Read device features (lower 32 bits) */
  uint32_t dev_features = virtio_read32(VIRTIO_LEGACY_HOST_FEATURES);

  /* Acknowledge + Driver */
  virtio_write8(VIRTIO_LEGACY_DEVICE_STATUS,
                VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER);

  /* Write guest features (subset we support) */
  uint32_t guest_features = VIRTIO_NET_GUEST_FEATURES & dev_features;
  virtio_write32(VIRTIO_LEGACY_GUEST_FEATURES, guest_features);

  /* Features OK */
  virtio_write8(VIRTIO_LEGACY_DEVICE_STATUS,
                VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK);

  /* Verify */
  uint8_t status = virtio_read8(VIRTIO_LEGACY_DEVICE_STATUS);
  if (!(status & VIRTIO_STATUS_FEATURES_OK)) {
    brights_print(&virtio_con, u"virtionet: feature negotiation failed\r\n");
    return -1;
  }

  return 0;
}

/* ---- Queue setup ---- */
static int virtionet_setup_queues(void)
{
  /* Receive queue (index 0) */
  if (virtq_init(&rx_virtq, 0, VIRTIO_MAX_QUEUE_SIZE) != 0) {
    brights_print(&virtio_con, u"virtionet: rx queue init failed\r\n");
    return -1;
  }

  /* Transmit queue (index 1) */
  if (virtq_init(&tx_virtq, 1, VIRTIO_MAX_QUEUE_SIZE) != 0) {
    brights_print(&virtio_con, u"virtionet: tx queue init failed\r\n");
    return -1;
  }

  brights_print(&virtio_con, u"virtionet: queues initialized\r\n");
  return 0;
}

/* ---- Post receive buffers ---- */
static int virtionet_post_rx_buf(uint8_t *buf)
{
  uint16_t idx;
  if (virtq_alloc_descriptor(&rx_virtq, &idx) != 0) return -1;

  uint32_t paddr = (uint32_t)(uintptr_t)buf;

  rx_virtq.desc[idx].addr = paddr;
  rx_virtq.desc[idx].len = RX_BUF_SIZE;
  rx_virtq.desc[idx].flags = VIRTQ_DESC_F_WRITE;
  rx_virtq.desc[idx].next = 0;

  virtq_add_avail(&rx_virtq, idx);
  return 0;
}

/* ---- Hardware init ---- */
static int virtionet_init_hw(void)
{
  /* Read MAC address from config space (offset 0x18 in legacy) */
  for (int i = 0; i < 6; ++i) {
    virtio_mac[i] = virtio_read8(VIRTIO_LEGACY_DEVICE_CFG + VIRTIO_NET_CONFIG_MAC + i);
  }

  brights_print(&virtio_con, u"virtionet: MAC ");
  const char *hx = "0123456789ABCDEF";
  for (int i = 0; i < 6; ++i) {
    char buf[3] = {hx[virtio_mac[i] >> 4], hx[virtio_mac[i] & 0xF], 0};
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, buf);
    if (i < 5) brights_serial_write_ascii(BRIGHTS_COM1_PORT, ":");
  }
  brights_print(&virtio_con, u"\r\n");

  /* Post initial receive buffers */
  for (int i = 0; i < RX_BUF_COUNT; ++i) {
    virtionet_post_rx_buf(rx_bufs[i]);
  }

  /* DRIVER_OK */
  virtio_write8(VIRTIO_LEGACY_DEVICE_STATUS,
                VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER |
                VIRTIO_STATUS_FEATURES_OK | VIRTIO_STATUS_DRIVER_OK);

  brights_print(&virtio_con, u"virtionet: ready\r\n");
  return 0;
}

/* ---- Send frame ---- */
static int virtionet_send_frame(const uint8_t *frame, uint32_t len)
{
  if (!frame || len == 0 || len > 1514) return -1;

  /* Reclaim any completed TX buffers */
  uint16_t idx;
  uint32_t rlen;
  while (virtq_get_used(&tx_virtq, &idx, &rlen) == 0) {
    virtq_free_descriptor(&tx_virtq, idx);
  }

  /* Get a descriptor (must be available after reclamation) */
  if (virtq_alloc_descriptor(&tx_virtq, &idx) != 0) return -1;

  /* Use pre-allocated DMA-safe buffer (not stack!) */
  uint8_t *tx_buf = tx_bufs[tx_next_buf % TX_BUF_COUNT];
  ++tx_next_buf;
  virtio_net_hdr_t *hdr = (virtio_net_hdr_t *)tx_buf;
  kutil_memset(hdr, 0, VIRTIO_NET_HDR_SIZE);
  kutil_memcpy(tx_buf + VIRTIO_NET_HDR_SIZE, frame, len);

  uint32_t total_len = VIRTIO_NET_HDR_SIZE + len;

  uint32_t paddr = (uint32_t)(uintptr_t)tx_buf;
  tx_virtq.desc[idx].addr = paddr;
  tx_virtq.desc[idx].len = total_len;
  tx_virtq.desc[idx].flags = 0;
  tx_virtq.desc[idx].next = 0;

  virtq_add_avail(&tx_virtq, idx);
  virtq_notify(&tx_virtq);

  return 0;
}

/* ---- Receive frame ---- */
static int virtionet_recv_frame(uint8_t *frame, uint32_t *len)
{
  if (!frame || !len) return -1;

  uint16_t idx;
  uint32_t rlen;
  if (virtq_get_used(&rx_virtq, &idx, &rlen) != 0) return -1;

  /* The descriptor was a write-only buffer for the device */
  virtq_desc_t *d = &rx_virtq.desc[idx];

  uint8_t *src = (uint8_t *)(uintptr_t)(uint32_t)d->addr;
  uint32_t data_len = rlen;

  /* Skip virtio-net header */
  if (data_len > VIRTIO_NET_HDR_SIZE) {
    uint32_t payload = data_len - VIRTIO_NET_HDR_SIZE;
    if (payload > *len) payload = *len;
    kutil_memcpy(frame, src + VIRTIO_NET_HDR_SIZE, payload);
    *len = payload;
  } else {
    *len = 0;
  }

  /* Repost the buffer */
  virtq_free_descriptor(&rx_virtq, idx);

  /* Re-post a fresh buffer */
  uint8_t *new_buf = rx_bufs[rx_next_buf % RX_BUF_COUNT];
  ++rx_next_buf;
  virtionet_post_rx_buf(new_buf);

  return (int)*len;
}

/* ---- Poll for RX activity ---- */
static int virtionet_poll_irq(void)
{
  /* Check if used ring has entries */
  return (rx_virtq.last_used_idx != rx_virtq.used->idx) ? 1 : 0;
}

/* ---- Registration ---- */
static brights_net_driver_t virtionet_driver;

int brights_virtionet_init(void)
{
  if (virtio_initialized) return 0;

  brights_serial_console_init(&virtio_con, BRIGHTS_COM1_PORT);

  if (virtionet_pci_find() != 0) {
    brights_print(&virtio_con, u"virtionet: no device found\r\n");
    return -1;
  }

  if (virtionet_map_pci() != 0) {
    brights_print(&virtio_con, u"virtionet: BAR setup failed\r\n");
    return -1;
  }

  if (virtionet_negotiate_features() != 0) {
    brights_print(&virtio_con, u"virtionet: feature negotiation failed\r\n");
    return -1;
  }

  if (virtionet_setup_queues() != 0) {
    brights_print(&virtio_con, u"virtionet: queue setup failed\r\n");
    return -1;
  }

  if (virtionet_init_hw() != 0) {
    brights_print(&virtio_con, u"virtionet: hw init failed\r\n");
    return -1;
  }

  /* Register with the net layer */
  virtionet_driver.name = "virtio-net";
  virtionet_driver.initialized = 1;
  virtionet_driver.ops.init = 0;
  virtionet_driver.ops.send = virtionet_send_frame;
  virtionet_driver.ops.recv = virtionet_recv_frame;
  virtionet_driver.ops.poll = virtionet_poll_irq;
  brights_net_register_driver(&virtionet_driver);

  virtio_initialized = 1;
  brights_print(&virtio_con, u"virtionet: init done\r\n");
  return 0;
}
