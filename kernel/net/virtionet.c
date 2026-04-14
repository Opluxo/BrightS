#include "virtionet.h"
#include "net.h"
#include "../kernel_util.h"
#include "../../drivers/serial.h"

#define VIRTIO_NET_HDR_SIZE 10

static int virtionet_initialized = 0;

#define VIRTIO_NET_S_LINKUP 1

typedef struct __attribute__((packed)) {
  uint8_t flags;
  uint8_t gso_type;
  uint16_t hdr_len;
  uint16_t gso_size;
  uint16_t csum_start;
  uint16_t csum_offset;
} virtio_net_hdr_t;

static int virtionet_pci_find(void)
{
    return 0;
}

static int virtionet_map_pci(void)
{
    return 0;
}

static int virtionet_negotiate_features(void)
{
    return 0;
}

static int virtionet_setup_queues(void)
{
    return 0;
}

static int virtionet_init_hw(void)
{
    return 0;
}

static int virtionet_send_frame(const uint8_t *frame, uint32_t len)
{
    if (!frame || len == 0 || len > 1514) return -1;
    
    uint8_t buffer[VIRTIO_NET_HDR_SIZE + 1514];
    virtio_net_hdr_t *hdr = (virtio_net_hdr_t *)buffer;
    
    hdr->flags = 0;
    hdr->gso_type = 0;
    hdr->hdr_len = VIRTIO_NET_HDR_SIZE;
    hdr->gso_size = 0;
    hdr->csum_start = 0;
    hdr->csum_offset = 0;
    
    uint8_t *data = buffer + VIRTIO_NET_HDR_SIZE;
    for (uint32_t i = 0; i < len; i++) {
        data[i] = frame[i];
    }
    
    return 0;
}

static int virtionet_recv_frame(uint8_t *frame, uint32_t *len)
{
    (void)frame;
    (void)len;
    return -1;
}

static int virtionet_poll_IRQ(void)
{
    return 0;
}

static brights_net_driver_t virtionet_driver;

int brights_virtionet_init(void)
{
    if (virtionet_initialized) return 0;
    
    if (virtionet_pci_find() != 0) {
        return -1;
    }
    
    if (virtionet_map_pci() != 0) {
        return -1;
    }
    
    if (virtionet_negotiate_features() != 0) {
        return -1;
    }
    
    if (virtionet_setup_queues() != 0) {
        return -1;
    }
    
    if (virtionet_init_hw() != 0) {
        return -1;
    }
    
    virtionet_initialized = 1;
    return 0;
}

int brights_virtionet_register(void)
{
    virtionet_driver.name = "virtio-net";
    virtionet_driver.initialized = 0;
    virtionet_driver.ops.init = virtionet_init_hw;
    virtionet_driver.ops.send = virtionet_send_frame;
    virtionet_driver.ops.recv = virtionet_recv_frame;
    virtionet_driver.ops.poll = virtionet_poll_IRQ;
    
    return brights_net_register_driver(&virtionet_driver);
}
