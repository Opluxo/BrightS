#ifndef BRIGHTS_VIRTIO_H
#define BRIGHTS_VIRTIO_H

#include <stdint.h>

/* Virtio PCI vendor */
#define VIRTIO_PCI_VENDOR 0x1AF4

/* Legacy virtio-net device IDs */
#define VIRTIO_DEVICE_NET_LEGACY 0x1000
#define VIRTIO_DEVICE_NET_MODERN 0x1041

/* Legacy MMIO register offsets from BAR0 */
#define VIRTIO_LEGACY_HOST_FEATURES  0x00
#define VIRTIO_LEGACY_GUEST_FEATURES 0x04
#define VIRTIO_LEGACY_QUEUE_ADDR     0x08
#define VIRTIO_LEGACY_QUEUE_SIZE     0x0C
#define VIRTIO_LEGACY_QUEUE_SEL      0x0E
#define VIRTIO_LEGACY_QUEUE_NOTIFY   0x10
#define VIRTIO_LEGACY_DEVICE_STATUS  0x12
#define VIRTIO_LEGACY_ISR_STATUS     0x14
#define VIRTIO_LEGACY_DEVICE_CFG     0x18

/* Device status flags */
#define VIRTIO_STATUS_ACKNOWLEDGE  1
#define VIRTIO_STATUS_DRIVER       2
#define VIRTIO_STATUS_DRIVER_OK    4
#define VIRTIO_STATUS_FEATURES_OK  8
#define VIRTIO_STATUS_FAILED      128

/* Common feature bits */
#define VIRTIO_F_NOTIFY_ON_EMPTY  (1ULL << 24)
#define VIRTIO_F_VERSION_1        (1ULL << 32)
#define VIRTIO_F_ACCESS_PLATFORM  (1ULL << 33)
#define VIRTIO_F_RING_PACKED      (1ULL << 34)
#define VIRTIO_F_IN_ORDER         (1ULL << 35)
#define VIRTIO_F_RING_EVENT_IDX   (1ULL << 29)

/* Virtio-net feature bits */
#define VIRTIO_NET_F_CSUM         (1ULL << 0)
#define VIRTIO_NET_F_GUEST_CSUM   (1ULL << 1)
#define VIRTIO_NET_F_MAC          (1ULL << 5)
#define VIRTIO_NET_F_GSO          (1ULL << 6)
#define VIRTIO_NET_F_GUEST_TSO4   (1ULL << 7)
#define VIRTIO_NET_F_GUEST_TSO6   (1ULL << 8)
#define VIRTIO_NET_F_GUEST_ECN    (1ULL << 9)
#define VIRTIO_NET_F_GUEST_UFO    (1ULL << 10)
#define VIRTIO_NET_F_HOST_TSO4    (1ULL << 11)
#define VIRTIO_NET_F_HOST_TSO6    (1ULL << 12)
#define VIRTIO_NET_F_HOST_ECN     (1ULL << 13)
#define VIRTIO_NET_F_HOST_UFO     (1ULL << 14)
#define VIRTIO_NET_F_MRG_RXBUF    (1ULL << 15)
#define VIRTIO_NET_F_STATUS       (1ULL << 16)
#define VIRTIO_NET_F_CTRL_VQ      (1ULL << 17)
#define VIRTIO_NET_F_CTRL_RX      (1ULL << 18)
#define VIRTIO_NET_F_CTRL_VLAN    (1ULL << 19)
#define VIRTIO_NET_F_GUEST_ANNOUNCE (1ULL << 21)
#define VIRTIO_NET_F_MQ           (1ULL << 22)
#define VIRTIO_NET_F_CTRL_MAC_ADDR (1ULL << 23)

/* Virtqueue descriptor flags */
#define VIRTQ_DESC_F_NEXT   1
#define VIRTQ_DESC_F_WRITE  2
#define VIRTQ_DESC_F_INDIRECT 4

/* Virtqueue descriptor (16 bytes) */
typedef struct __attribute__((packed)) {
  uint64_t addr;
  uint32_t len;
  uint16_t flags;
  uint16_t next;
} virtq_desc_t;

/* Virtqueue available ring */
typedef struct __attribute__((packed)) {
  uint16_t flags;
  uint16_t idx;
  uint16_t ring[];
} virtq_avail_t;

/* Virtqueue used ring element */
typedef struct __attribute__((packed)) {
  uint32_t id;
  uint32_t len;
} virtq_used_elem_t;

/* Virtqueue used ring */
typedef struct __attribute__((packed)) {
  uint16_t flags;
  uint16_t idx;
  virtq_used_elem_t ring[];
} virtq_used_t;

/* Virtio-net header (10 bytes, prepended to each packet) */
typedef struct __attribute__((packed)) {
  uint8_t flags;
  uint8_t gso_type;
  uint16_t hdr_len;
  uint16_t gso_size;
  uint16_t csum_start;
  uint16_t csum_offset;
} virtio_net_hdr_t;

#define VIRTIO_NET_HDR_SIZE sizeof(virtio_net_hdr_t)

/* Virtio-net config space offsets (legacy, from device config at 0x18) */
#define VIRTIO_NET_CONFIG_MAC        0x00
#define VIRTIO_NET_CONFIG_STATUS     0x06
#define VIRTIO_NET_CONFIG_MAX_VQPAIR 0x08

/* Virtqueue structure (software tracking) */
#define VIRTIO_MAX_QUEUE_SIZE 256

typedef struct {
  uint16_t queue_size;
  uint16_t free_head;
  uint16_t avail_idx;
  uint16_t last_used_idx;

  /* Physical address of the descriptor area (3 parts contiguous) */
  uint32_t desc_paddr;
  uint32_t avail_paddr;
  uint32_t used_paddr;

  virtq_desc_t *desc;
  virtq_avail_t *avail;
  virtq_used_t *used;

  /* Free descriptor stack */
  uint16_t free_stack[VIRTIO_MAX_QUEUE_SIZE];
  uint16_t free_count;
} virtq_t;

#endif
