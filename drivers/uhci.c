#include "uhci.h"
#ifdef __i386__
#include "../arch/i386/io.h"
#else
#include "../arch/x86_64/io.h"
#endif
#include "../include/kernel/kmalloc.h"
#include "../kernel/core/printf.h"
#include "serial.h"

static void uhci_debug(const char *msg)
{
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, msg);
}

/* Wait for UHCI controller to halt */
static int uhci_wait_halt(uhci_controller_t *uhci, int timeout_us)
{
  for (int i = 0; i < timeout_us; ++i) {
    if (inw(uhci->iobase + UHCI_USBSTS) & UHCI_STS_HCHALTED) return 0;
    __asm__ __volatile__("pause");
  }
  return -1;
}

/* Allocate a TD from the pool */
static uhci_td_t *uhci_td_alloc(uhci_controller_t *uhci)
{
  if (uhci->free_tds) {
    uhci_td_t *td = uhci->free_tds;
    uhci->free_tds = (uhci_td_t *)(uintptr_t)(td->link & ~0xF);
    return td;
  }
  /* Allocate a new page of TDs */
  uhci_td_t *page = (uhci_td_t *)brights_kmalloc(4096);
  if (!page) return 0;
  /* Link all TDs in the page into free list */
  int count = 4096 / sizeof(uhci_td_t);
  for (int i = 0; i < count - 1; ++i) {
    page[i].link = (uint32_t)(uintptr_t)&page[i + 1];
  }
  page[count - 1].link = (uint32_t)(uintptr_t)uhci->free_tds;
  uhci->free_tds = page;
  return uhci_td_alloc(uhci);
}

/* Free a TD back to the pool */
static void uhci_td_free(uhci_controller_t *uhci, uhci_td_t *td)
{
  td->link = (uint32_t)(uintptr_t)uhci->free_tds;
  uhci->free_tds = td;
}

/* Initialize a TD for a specific transfer */
static void uhci_td_setup(uhci_td_t *td, uint32_t pid, uint8_t dev_addr,
  uint8_t ep, int toggle, uint8_t *buf, uint32_t len, int ioc)
{
  td->link = UHCI_TD_TERMINATE;
  td->status = UHCI_TD_ACTIVE | UHCI_TD_SPD;
  if (ioc) td->status |= UHCI_TD_IOC;
  td->token = UHCI_TOKEN_PACKET_ID(pid) |
              UHCI_TOKEN_DEV_ADDR(dev_addr) |
              UHCI_TOKEN_ENDPOINT(ep) |
              UHCI_TOKEN_TOGGLE(toggle) |
              UHCI_TOKEN_MAXLEN(len);
  td->buffer = (uint32_t)(uintptr_t)buf;
}

/* Wait for a TD to complete (polling, no interrupt) */
static int uhci_td_wait(uhci_td_t *td, int timeout_us)
{
  for (int i = 0; i < timeout_us; ++i) {
    if (!(td->status & UHCI_TD_ACTIVE)) {
      if (td->status & UHCI_TD_STALLED) return -2;
      if (td->status & UHCI_TD_BABBLE)  return -3;
      if (td->status & UHCI_TD_TIMEOUT) return -4;
      if (td->status & UHCI_TD_NAK)     return -5;
      return 0;
    }
    __asm__ __volatile__("pause");
  }
  return -1;
}

/* Do a complete control transfer: SETUP -> DATA (optional) -> STATUS */
int brights_uhci_ctrl_transfer(usb_controller_t *ctrl, uint8_t dev_addr,
  uint8_t ep, usb_device_request_t *req, void *data)
{
  uhci_controller_t *uhci = (uhci_controller_t *)ctrl;
  uhci_td_t *setup_td = uhci_td_alloc(uhci);
  uhci_td_t *status_td = uhci_td_alloc(uhci);
  uhci_td_t *data_td = 0;

  if (!setup_td || !status_td) {
    if (setup_td) uhci_td_free(uhci, setup_td);
    if (status_td) uhci_td_free(uhci, status_td);
    return -1;
  }

  /* SETUP TD */
  uhci_td_setup(setup_td, USB_PID_SETUP, dev_addr, ep, 0,
    (uint8_t *)req, sizeof(*req), 0);

  int dir_in = (req->bmRequestType & 0x80) ? 1 : 0;
  uint16_t wLen = req->wLength;

  /* DATA TD (if any) */
  if (wLen > 0 && data) {
    data_td = uhci_td_alloc(uhci);
    if (!data_td) {
      uhci_td_free(uhci, setup_td);
      uhci_td_free(uhci, status_td);
      return -1;
    }
    uhci_td_setup(data_td, dir_in ? USB_PID_IN : USB_PID_OUT,
      dev_addr, ep, 1, (uint8_t *)data, wLen, 0);
    setup_td->link = (uint32_t)(uintptr_t)data_td;
  }

  /* STATUS TD (opposite direction, toggle=1) */
  uhci_td_setup(status_td, dir_in ? USB_PID_OUT : USB_PID_IN,
    dev_addr, ep, 1, 0, 0, 1);

  if (data_td) {
    data_td->link = (uint32_t)(uintptr_t)status_td;
  } else {
    setup_td->link = (uint32_t)(uintptr_t)status_td;
  }

  /* Queue via the async QH */
  uhci->async_qh->element = (uint32_t)(uintptr_t)setup_td;

  /* Wait for completion */
  int ret = uhci_td_wait(status_td, 500000);

  /* Clean up */
  uhci_td_free(uhci, setup_td);
  if (data_td) uhci_td_free(uhci, data_td);
  uhci_td_free(uhci, status_td);
  uhci->async_qh->element = UHCI_TD_TERMINATE;

  return ret;
}

/* Bulk transfer */
int brights_uhci_bulk_transfer(usb_controller_t *ctrl, uint8_t dev_addr,
  uint8_t ep, void *data, uint32_t len, int dir_in)
{
  uhci_controller_t *uhci = (uhci_controller_t *)ctrl;
  uint32_t max_pkt = 64;
  int toggle = 0;
  int remaining = (int)len;
  uint8_t *buf = (uint8_t *)data;

  while (remaining > 0) {
    uint32_t chunk = remaining > (int)max_pkt ? max_pkt : (uint32_t)remaining;

    uhci_td_t *td = uhci_td_alloc(uhci);
    if (!td) return -1;

    uhci_td_setup(td, dir_in ? USB_PID_IN : USB_PID_OUT,
      dev_addr, ep & 0x0F, toggle, buf, chunk, 0);
    td->link = UHCI_TD_TERMINATE;

    uhci->async_qh->element = (uint32_t)(uintptr_t)td;

    int ret = uhci_td_wait(td, 500000);
    if (ret != 0) {
      uhci->async_qh->element = UHCI_TD_TERMINATE;
      uhci_td_free(uhci, td);
      return ret;
    }

    int actual = (td->status >> 21) & 0x7FF;
    if (actual == 0x7FF) actual = 0;
    else actual = chunk - actual;

    uhci->async_qh->element = UHCI_TD_TERMINATE;
    uhci_td_free(uhci, td);

    remaining -= actual;
    buf += actual;
    toggle = 1 - toggle;

    if (actual < (int)chunk) break;
  }

  return len - remaining;
}

/* Interrupt transfer (simplified: synchronous poll) */
int brights_uhci_int_transfer(usb_controller_t *ctrl, uint8_t dev_addr,
  uint8_t ep, void *data, uint32_t len, int dir_in)
{
  uhci_controller_t *uhci = (uhci_controller_t *)ctrl;

  uhci_td_t *td = uhci_td_alloc(uhci);
  if (!td) return -1;

  int toggle = uhci->int_toggle[dev_addr] ? 1 : 0;
  uhci_td_setup(td, dir_in ? USB_PID_IN : USB_PID_OUT,
    dev_addr, ep & 0x0F, toggle, (uint8_t *)data, len, 0);

  /* Add to async QH */
  uhci->async_qh->element = (uint32_t)(uintptr_t)td;

  int ret = uhci_td_wait(td, 500000);

  int actual = 0;
  if (ret == 0) {
    actual = (td->status >> 21) & 0x7FF;
    if (actual == 0x7FF) actual = 0;
    else actual = len - actual;
    uhci->int_toggle[dev_addr] = 1 - uhci->int_toggle[dev_addr];
  }

  uhci->async_qh->element = UHCI_TD_TERMINATE;
  uhci_td_free(uhci, td);

  return ret == 0 ? actual : ret;
}

/* Initialize a UHCI controller from PCI device */
int brights_uhci_init(uhci_controller_t *uhci, const brights_pci_device_t *pci_dev)
{
  /* Get I/O base from BAR4 (UHCI uses I/O space, typically BAR4) */
  uint32_t bar = pci_dev->bar[4];
  if ((bar & 1) == 0) {
    /* Try BAR0 if BAR4 isn't I/O */
    bar = pci_dev->bar[0];
    if ((bar & 1) == 0) return -1;
  }
  uhci->iobase = bar & ~0x3;

  uhci->base.index = 0;
  uhci->base.type = USB_CONTROLLER_UHCI;
  uhci->base.vendor_id = pci_dev->vendor_id;
  uhci->base.device_id = pci_dev->device_id;
  uhci->base.prog_if = pci_dev->prog_if;
  uhci->base.iobase = uhci->iobase;
  uhci->base.mmio_base = 0;
  uhci->base.irq = 0;
  uhci->base.ctrl_transfer = brights_uhci_ctrl_transfer;
  uhci->base.bulk_transfer = brights_uhci_bulk_transfer;
  uhci->base.int_transfer = brights_uhci_int_transfer;
  uhci->num_ports = 2;
  uhci->free_tds = 0;
  uhci->async_qh = 0;
  uhci->frame_list = 0;
  for (int i = 0; i < 128; ++i) uhci->int_toggle[i] = 0;

  /* Reset controller */
  outw(uhci->iobase + UHCI_USBCMD, UHCI_CMD_HCRESET);
  if (uhci_wait_halt(uhci, 10000) != 0) {
    uhci_debug("uhci: reset failed\r\n");
    return -1;
  }

  /* Allocate frame list (4KB, page-aligned) */
  uhci->frame_list = (uint32_t *)brights_kmalloc(4096);
  if (!uhci->frame_list) {
    uhci_debug("uhci: frame_list alloc failed\r\n");
    return -1;
  }
  for (int i = 0; i < UHCI_FRAME_LIST_COUNT; ++i) {
    uhci->frame_list[i] = UHCI_TD_TERMINATE;
  }

  /* Allocate async QH */
  uhci->async_qh = (uhci_qh_t *)brights_kmalloc(32);
  if (!uhci->async_qh) {
    uhci_debug("uhci: async_qh alloc failed\r\n");
    brights_kfree(uhci->frame_list);
    return -1;
  }
  uhci->async_qh->head_link = UHCI_TD_TERMINATE;
  uhci->async_qh->element = UHCI_TD_TERMINATE;

  /* Point all frame list entries to async QH */
  for (int i = 0; i < UHCI_FRAME_LIST_COUNT; ++i) {
    uhci->frame_list[i] = (uint32_t)(uintptr_t)uhci->async_qh | UHCI_TD_QH;
  }

  /* Set frame list base address */
  outl(uhci->iobase + UHCI_FRBASEADD, (uint32_t)(uintptr_t)uhci->frame_list);

  /* Set SOF modify (default 64 = 1ms frames) */
  outb(uhci->iobase + UHCI_SOFMOD, 64);

  /* Clear status bits */
  outw(uhci->iobase + UHCI_USBSTS, 0x001F);

  /* Start controller (run, configure flag, max packet = 64) */
  outw(uhci->iobase + UHCI_USBCMD, UHCI_CMD_RUN | UHCI_CMD_MAXP | UHCI_CMD_CF);

  /* Enable interrupts */
  outw(uhci->iobase + UHCI_USBINTR, 0x0001);

  /* Determine number of ports by reading PORTSC registers */
  uhci->num_ports = 0;
  for (int i = 0; i < 8; ++i) {
    uint16_t portsc = inw(uhci->iobase + UHCI_PORTSC1 + i * 2);
    if (portsc == 0xFFFF || portsc == 0) break;
    uhci->num_ports = i + 1;
  }

  uhci_debug("uhci: init ok, iobase=0x");
  {
    const char *hex = "0123456789ABCDEF";
    char buf[12];
    int bi = 0;
    buf[bi++] = '0'; buf[bi++] = 'x';
    uint32_t v = uhci->iobase;
    for (int s = 28; s >= 0; s -= 4) buf[bi++] = hex[(v >> s) & 0xF];
    buf[bi] = 0;
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, buf);
  }
  uhci_debug(" ports=");
  {
    char c[4];
    int n = uhci->num_ports;
    c[0] = '0' + n;
    c[1] = '\r'; c[2] = '\n'; c[3] = 0;
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, c);
  }

  return 0;
}
