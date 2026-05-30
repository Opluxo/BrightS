#ifndef BRIGHTS_UHCI_H
#define BRIGHTS_UHCI_H

#include <stdint.h>
#ifdef __i386__
#include "../arch/i386/pci.h"
#else
#include "../arch/x86_64/pci.h"
#endif
#include "usb.h"

/* UHCI I/O register offsets */
#define UHCI_USBCMD       0x00
#define UHCI_USBSTS       0x02
#define UHCI_USBINTR      0x04
#define UHCI_FRNUM        0x06
#define UHCI_FRBASEADD    0x08
#define UHCI_SOFMOD       0x0C
#define UHCI_PORTSC1      0x10
#define UHCI_PORTSC2      0x12

/* USBCMD bits */
#define UHCI_CMD_RUN      (1 << 0)
#define UHCI_CMD_HCRESET  (1 << 1)
#define UHCI_CMD_GRESET   (1 << 2)
#define UHCI_CMD_GLOBALSW (1 << 3)
#define UHCI_CMD_MAXP     (1 << 7)
#define UHCI_CMD_CF       (1 << 6)
#define UHCI_CMD_SWDBG    (1 << 5)

/* USBSTS bits */
#define UHCI_STS_USBINT   (1 << 0)
#define UHCI_STS_ERROR    (1 << 1)
#define UHCI_STS_RD       (1 << 2)
#define UHCI_STS_HSE      (1 << 3)
#define UHCI_STS_HCPE     (1 << 4)
#define UHCI_STS_HCHALTED (1 << 5)

/* PORTSC bits */
#define UHCI_PORTSC_CCS       (1 << 0)
#define UHCI_PORTSC_CSC       (1 << 1)
#define UHCI_PORTSC_PE        (1 << 2)
#define UHCI_PORTSC_ENC       (1 << 3)
#define UHCI_PORTSC_SUSP      (1 << 12)
#define UHCI_PORTSC_PR        (1 << 9)
#define UHCI_PORTSC_LSDA      (1 << 8)
#define UHCI_PORTSC_RD        (1 << 10)
#define UHCI_PORTSC_RESET      (1 << 9)
#define UHCI_PORTSC_RESUME    (1 << 6)
#define UHCI_PORTSC_OCIC      (1 << 3)

/* UHCI Transfer Descriptor (32 bytes, must be 16-byte aligned) */
typedef struct {
  uint32_t link;
  uint32_t status;
  uint32_t token;
  uint32_t buffer;
} __attribute__((packed)) uhci_td_t;

/* UHCI Queue Head (20 bytes, must be 16-byte aligned) */
typedef struct {
  uint32_t head_link;
  uint32_t element;
} __attribute__((packed)) uhci_qh_t;

/* TD link pointer bits */
#define UHCI_TD_TERMINATE    (1 << 0)
#define UHCI_TD_QH           (1 << 1)
#define UHCI_TD_DEPTH        (1 << 2)

/* TD status bits */
#define UHCI_TD_ACTIVE       (1 << 23)
#define UHCI_TD_STALLED      (1 << 22)
#define UHCI_TD_DBUFERR      (1 << 21)
#define UHCI_TD_BABBLE       (1 << 20)
#define UHCI_TD_NAK          (1 << 19)
#define UHCI_TD_TIMEOUT      (1 << 18)
#define UHCI_TD_SPD          (1 << 29)
#define UHCI_TD_IOC          (1 << 24)

/* TD token bits */
#define UHCI_TOKEN_PID_SHIFT 0
#define UHCI_TOKEN_PID_MASK  (0xFF << 0)
#define UHCI_TOKEN_DEV_SHIFT 8
#define UHCI_TOKEN_DEV_MASK  (0x7F << 8)
#define UHCI_TOKEN_EP_SHIFT  15
#define UHCI_TOKEN_EP_MASK   (0xF << 15)
#define UHCI_TOKEN_TOGGLE_SHIFT 19
#define UHCI_TOKEN_TOGGLE_MASK  (1 << 19)
#define UHCI_TOKEN_MAXLEN_SHIFT 21
#define UHCI_TOKEN_MAXLEN_MASK  (0x7FF << 21)

#define UHCI_TOKEN_PACKET_ID(pid)       ((pid) << UHCI_TOKEN_PID_SHIFT)
#define UHCI_TOKEN_DEV_ADDR(addr)       ((addr) << UHCI_TOKEN_DEV_SHIFT)
#define UHCI_TOKEN_ENDPOINT(ep)         ((ep) << UHCI_TOKEN_EP_SHIFT)
#define UHCI_TOKEN_TOGGLE(t)            ((t) << UHCI_TOKEN_TOGGLE_SHIFT)
#define UHCI_TOKEN_MAXLEN(len)          (((len) - 1) << UHCI_TOKEN_MAXLEN_SHIFT)

#define UHCI_FRAME_LIST_COUNT 1024

/* UHCI controller state */
typedef struct {
  usb_controller_t base;
  uint32_t iobase;
  uint32_t *frame_list;    /* physical address of frame list (4KB aligned) */
  uhci_qh_t  *async_qh;    /* Queue head for async (control/bulk) transfers */
  int         num_ports;
  uint8_t     device_addr[2]; /* address assigned to devices on each port */
  int         td_pool_count;
  uhci_td_t   *td_pool[64];
  uhci_td_t   *free_tds;
  uint8_t     int_toggle[128];
} uhci_controller_t;

/* UHCI functions */
int brights_uhci_init(uhci_controller_t *uhci, const brights_pci_device_t *pci_dev);
int brights_uhci_ctrl_transfer(usb_controller_t *ctrl, uint8_t dev_addr,
  uint8_t ep, usb_device_request_t *req, void *data);
int brights_uhci_bulk_transfer(usb_controller_t *ctrl, uint8_t dev_addr,
  uint8_t ep, void *data, uint32_t len, int dir_in);
int brights_uhci_int_transfer(usb_controller_t *ctrl, uint8_t dev_addr,
  uint8_t ep, void *data, uint32_t len, int dir_in);

#endif
