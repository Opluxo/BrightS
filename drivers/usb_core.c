#include "usb.h"
#include "uhci.h"
#ifdef __i386__
#include "../arch/i386/io.h"
#else
#include "../arch/x86_64/io.h"
#endif
#include "../kernel/core/printf.h"
#include "../include/kernel/kmalloc.h"
#include "serial.h"

#define USB_MAX_CONTROLLERS 4
#define USB_MAX_DEVICES 32

static usb_controller_t *controllers[USB_MAX_CONTROLLERS];
static int controller_count = 0;

static usb_device_t devices[USB_MAX_DEVICES];
static int device_count = 0;

/* MSC state */
static int usb_msc_ready = 0;
static usb_device_t *msc_device = 0;

static void usb_debug(const char *msg)
{
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, msg);
}

static void usb_debug_hex(uint32_t v)
{
  const char *hex = "0123456789ABCDEF";
  char buf[12];
  int i = 0;
  buf[i++] = '0'; buf[i++] = 'x';
  for (int s = 28; s >= 0; s -= 4) buf[i++] = hex[(v >> s) & 0xF];
  buf[i] = 0;
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, buf);
}

/* Allocate a new USB address for a device */
static uint8_t usb_alloc_address(void)
{
  for (uint8_t addr = 1; addr < 128; ++addr) {
    int found = 0;
    for (int i = 0; i < device_count; ++i) {
      if (devices[i].address == addr) { found = 1; break; }
    }
    if (!found) return addr;
  }
  return 0;
}

/* Add a device to the device list */
static int usb_add_device(usb_device_t *dev)
{
  if (device_count >= USB_MAX_DEVICES) return -1;
  devices[device_count++] = *dev;
  return device_count - 1;
}

/* Standard control transfer wrapper */
static int usb_std_ctrl_transfer(usb_controller_t *ctrl, uint8_t dev_addr,
  uint8_t bmReqType, uint8_t bRequest, uint16_t wValue,
  uint16_t wIndex, uint16_t wLength, void *data)
{
  usb_device_request_t req;
  req.bmRequestType = bmReqType;
  req.bRequest = bRequest;
  req.wValue = wValue;
  req.wIndex = wIndex;
  req.wLength = wLength;

  if (!ctrl || !ctrl->ctrl_transfer) return -1;
  return ctrl->ctrl_transfer(ctrl, dev_addr, 0, &req, data);
}

/* Get device descriptor */
static int usb_get_device_descriptor(usb_controller_t *ctrl, uint8_t dev_addr,
  usb_device_descriptor_t *desc)
{
  return usb_std_ctrl_transfer(ctrl, dev_addr,
    USB_REQ_TYPE_DEVICE_TO_HOST | USB_REQ_TYPE_STANDARD | USB_REQ_RECIPIENT_DEVICE,
    USB_REQ_GET_DESCRIPTOR, USB_DESC_DEVICE << 8, 0, sizeof(*desc), desc);
}

/* Set device address */
static int usb_set_address(usb_controller_t *ctrl, uint8_t dev_addr, uint8_t new_addr)
{
  return usb_std_ctrl_transfer(ctrl, dev_addr,
    USB_REQ_TYPE_HOST_TO_DEVICE | USB_REQ_TYPE_STANDARD | USB_REQ_RECIPIENT_DEVICE,
    USB_REQ_SET_ADDRESS, new_addr, 0, 0, 0);
}

/* Get full configuration descriptor (includes interface + endpoint descriptors) */
static int usb_get_full_config_desc(usb_controller_t *ctrl, uint8_t dev_addr,
  uint8_t config_idx, void *buf, uint16_t len)
{
  return usb_std_ctrl_transfer(ctrl, dev_addr,
    USB_REQ_TYPE_DEVICE_TO_HOST | USB_REQ_TYPE_STANDARD | USB_REQ_RECIPIENT_DEVICE,
    USB_REQ_GET_DESCRIPTOR, (USB_DESC_CONFIGURATION << 8) | config_idx,
    0, len, buf);
}

/* Set configuration */
static int usb_set_configuration(usb_controller_t *ctrl, uint8_t dev_addr,
  uint8_t config_val)
{
  return usb_std_ctrl_transfer(ctrl, dev_addr,
    USB_REQ_TYPE_HOST_TO_DEVICE | USB_REQ_TYPE_STANDARD | USB_REQ_RECIPIENT_DEVICE,
    USB_REQ_SET_CONFIGURATION, config_val, 0, 0, 0);
}

/* Set HID protocol (boot vs report) */
static int usb_hid_set_protocol(usb_controller_t *ctrl, uint8_t dev_addr,
  uint8_t interface, uint8_t protocol)
{
  return usb_std_ctrl_transfer(ctrl, dev_addr,
    USB_REQ_TYPE_HOST_TO_DEVICE | USB_REQ_TYPE_CLASS | USB_REQ_RECIPIENT_INTERFACE,
    0x0B, protocol, interface, 0, 0);
}

/* Set HID idle rate */
static int usb_hid_set_idle(usb_controller_t *ctrl, uint8_t dev_addr,
  uint8_t interface, uint8_t duration)
{
  return usb_std_ctrl_transfer(ctrl, dev_addr,
    USB_REQ_TYPE_HOST_TO_DEVICE | USB_REQ_TYPE_CLASS | USB_REQ_RECIPIENT_INTERFACE,
    0x0A, (duration << 8), interface, 0, 0);
}

/* Enumerate a single USB device */
static int usb_enumerate_device(usb_controller_t *ctrl, int port, uint8_t speed)
{
  usb_device_descriptor_t dev_desc;
  uint8_t new_addr;

  /* Get initial device descriptor (uses default address 0) */
  if (usb_get_device_descriptor(ctrl, 0, &dev_desc) != 0) {
    usb_debug("usb: enum failed - get dev desc\r\n");
    return -1;
  }

  new_addr = usb_alloc_address();
  if (new_addr == 0) {
    usb_debug("usb: no addresses available\r\n");
    return -1;
  }

  /* Set address */
  if (usb_set_address(ctrl, 0, new_addr) != 0) {
    usb_debug("usb: set address failed\r\n");
    return -1;
  }

  /* Get descriptor again at new address */
  if (usb_get_device_descriptor(ctrl, new_addr, &dev_desc) != 0) {
    usb_debug("usb: get desc after setaddr failed\r\n");
    return -1;
  }

  /* Create device entry */
  usb_device_t dev;
  dev.address = new_addr;
  dev.speed = speed;
  dev.port = (uint8_t)port;
  dev.hub_port = 0;
  dev.controller_idx = ctrl->index;
  dev.configured = 0;
  dev.class_code = dev_desc.bDeviceClass;
  dev.sub_class = dev_desc.bDeviceSubClass;
  dev.protocol = dev_desc.bDeviceProtocol;
  dev.vendor_id = dev_desc.idVendor;
  dev.product_id = dev_desc.idProduct;
  dev.max_packet_size = dev_desc.bMaxPacketSize0;
  dev.num_configs = dev_desc.bNumConfigurations;
  dev.num_interfaces = 0;
  dev.ep_count = 0;

  usb_debug("usb: device ");
  usb_debug_hex(new_addr);
  usb_debug(" vid=");
  usb_debug_hex(dev_desc.idVendor);
  usb_debug(" pid=");
  usb_debug_hex(dev_desc.idProduct);
  usb_debug(" class=");
  usb_debug_hex(dev_desc.bDeviceClass);
  usb_debug("\r\n");

  /* Read configuration to find interfaces */
  uint8_t config_buf[256];
  usb_config_descriptor_t *cfg_desc = (usb_config_descriptor_t *)config_buf;

  if (usb_get_full_config_desc(ctrl, new_addr, 0, config_buf, sizeof(config_buf)) == 0) {
    uint16_t total_len = cfg_desc->wTotalLength;
    if (total_len > sizeof(config_buf)) total_len = sizeof(config_buf);

    /* Parse through descriptors to find interfaces and endpoints */
    uint8_t *p = config_buf;
    uint8_t *end = p + total_len;
    uint8_t iface_num = 0;

    while (p + 2 <= end) {
      uint8_t len = p[0];
      uint8_t type = p[1];
      if (len == 0) break;
      if (p + len > end) break;

      if (type == USB_DESC_INTERFACE && len >= 9) {
        usb_interface_descriptor_t *iface = (usb_interface_descriptor_t *)p;
        if (iface->bInterfaceNumber < 8) {
          dev.interface_class[iface_num] = iface->bInterfaceClass;
          dev.interface_subclass[iface_num] = iface->bInterfaceSubClass;
          dev.interface_protocol[iface_num] = iface->bInterfaceProtocol;
          dev.num_interfaces = iface_num + 1;
        }
        iface_num++;
      }

      if (type == USB_DESC_ENDPOINT && len >= 7) {
        usb_endpoint_descriptor_t *ep = (usb_endpoint_descriptor_t *)p;
        if (dev.ep_count < 8) {
          dev.ep_in_addr[dev.ep_count] = ep->bEndpointAddress;
          dev.ep_out_addr[dev.ep_count] = ep->bEndpointAddress;
          dev.ep_max_packet[dev.ep_count] = ep->wMaxPacketSize;
          dev.ep_type[dev.ep_count] = ep->bmAttributes & 0x03;
          dev.ep_count++;
        }
      }

      p += len;
    }
  }

  /* Set configuration */
  if (usb_set_configuration(ctrl, new_addr, 1) != 0) {
    usb_debug("usb: set config failed\r\n");
    return -1;
  }
  dev.configured = 1;

  /* Add to device list */
  int idx = usb_add_device(&dev);

  usb_debug("usb: device configured, addr=");
  usb_debug_hex(new_addr);
  usb_debug(" interfaces=");
  {
    char c[4];
    int n = dev.num_interfaces;
    c[0] = '0' + (n / 10);
    c[1] = '0' + (n % 10);
    c[2] = '\r'; c[3] = '\n';
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, c);
  }

  return idx;
}

/* Initialize a USB HID keyboard */
static int usb_init_hid_keyboard(usb_controller_t *ctrl, usb_device_t *dev)
{
  /* Find HID interface and interrupt IN endpoint */
  uint8_t int_in_ep = 0;
  int hid_iface = -1;

  for (int i = 0; i < dev->num_interfaces && i < 8; ++i) {
    if (dev->interface_class[i] == USB_CLASS_HID) {
      hid_iface = i;
      break;
    }
  }

  if (hid_iface < 0) return -1;

  /* Find interrupt IN endpoint */
  for (int i = 0; i < dev->ep_count; ++i) {
    if (dev->ep_type[i] == USB_EP_INTERRUPT && (dev->ep_in_addr[i] & 0x80)) {
      int_in_ep = dev->ep_in_addr[i];
      break;
    }
  }

  if (int_in_ep == 0) return -1;

  usb_debug("usb: HID keyboard found, ep=0x");
  usb_debug_hex(int_in_ep);
  usb_debug("\r\n");

  /* Set boot protocol for keyboard */
  usb_hid_set_protocol(ctrl, dev->address, (uint8_t)hid_iface, 1);
  usb_hid_set_idle(ctrl, dev->address, (uint8_t)hid_iface, 0);

  return 0;
}

/* Initialize USB mass storage */
static int usb_init_msc(usb_controller_t *ctrl, usb_device_t *dev)
{
  int is_msc = 0;
  for (int i = 0; i < dev->num_interfaces && i < 8; ++i) {
    if (dev->interface_class[i] == USB_CLASS_MASS_STORAGE) {
      is_msc = 1;
      break;
    }
  }

  if (!is_msc) return -1;

  usb_debug("usb: MSC device found\r\n");
  msc_device = dev;

  if (brights_usb_msc_init(ctrl, dev) != 0) {
    usb_debug("usb: MSC init failed\r\n");
    return -1;
  }

  usb_msc_ready = 1;
  return 0;
}

/* Enumerate all devices on a controller */
static void usb_enumerate_controller(usb_controller_t *ctrl)
{
  if (!ctrl || !ctrl->ctrl_transfer) return;

  usb_debug("usb: enumerating controller ");
  usb_debug_hex(ctrl->index);
  usb_debug("\r\n");

  /* For each port, check for connected device and enumerate */
  uhci_controller_t *uhci = (uhci_controller_t *)ctrl;

  for (int port = 0; port < uhci->num_ports; ++port) {
    uint16_t portsc = inw(uhci->iobase + UHCI_PORTSC1 + port * 2);

    if (portsc & UHCI_PORTSC_CCS) {
      uint8_t speed = (portsc & UHCI_PORTSC_LSDA) ? USB_SPEED_LOW : USB_SPEED_FULL;

      usb_debug("usb: port ");
      {
        char c[4];
        c[0] = '0' + port;
        c[1] = '\r'; c[2] = '\n'; c[3] = 0;
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, c);
      }

      /* Reset port */
      outw(uhci->iobase + UHCI_PORTSC1 + port * 2, UHCI_PORTSC_PR);
      for (volatile int i = 0; i < 50000; ++i) { __asm__ __volatile__("pause"); }
      outw(uhci->iobase + UHCI_PORTSC1 + port * 2, 0);
      for (volatile int i = 0; i < 50000; ++i) { __asm__ __volatile__("pause"); }

      /* Wait for port to become enabled */
      portsc = inw(uhci->iobase + UHCI_PORTSC1 + port * 2);

      int idx = usb_enumerate_device(ctrl, port, speed);
      if (idx >= 0) {
        usb_device_t *dev = &devices[idx];
        if (dev->class_code == USB_CLASS_HID ||
            (dev->num_interfaces > 0 && dev->interface_class[0] == USB_CLASS_HID)) {
          usb_init_hid_keyboard(ctrl, dev);
        }
        if (dev->class_code == USB_CLASS_MASS_STORAGE ||
            (dev->num_interfaces > 0 && dev->interface_class[0] == USB_CLASS_MASS_STORAGE)) {
          usb_init_msc(ctrl, dev);
        }
      }
    }
  }
}

/* Initialize USB subsystem */
int brights_usb_init(void)
{
  brights_pci_device_t pci_devs[64];
  int count = brights_pci_scan(pci_devs, 64);

  if (count <= 0) {
    usb_debug("usb: pci scan failed\r\n");
    return -1;
  }

  controller_count = 0;

  /* Find UHCI controllers (class=0x0C, subclass=0x03, prog_if=0x00) */
  for (int i = 0; i < count && controller_count < USB_MAX_CONTROLLERS; ++i) {
    if (pci_devs[i].class_code == 0x0C && pci_devs[i].subclass == 0x03) {
      usb_debug("usb: found USB controller at ");
      usb_debug_hex(pci_devs[i].bus);
      usb_debug(":");
      usb_debug_hex(pci_devs[i].dev);
      usb_debug(".");
      usb_debug_hex(pci_devs[i].func);
      usb_debug(" prog_if=");
      usb_debug_hex(pci_devs[i].prog_if);
      usb_debug("\r\n");

      if (pci_devs[i].prog_if == 0x00) {
        /* UHCI controller - use I/O BAR */
        uhci_controller_t *uhci = (uhci_controller_t *)brights_kmalloc(sizeof(uhci_controller_t));
        if (!uhci) {
          usb_debug("usb: alloc failed\r\n");
          continue;
        }

        if (brights_uhci_init(uhci, &pci_devs[i]) == 0) {
          controllers[controller_count] = &uhci->base;
          controller_count++;
          usb_debug("usb: UHCI init ok\r\n");
        } else {
          usb_debug("usb: UHCI init failed\r\n");
          brights_kfree(uhci);
        }
      } else if (pci_devs[i].prog_if == 0x20) {
        usb_debug("usb: EHCI not yet supported\r\n");
      } else if (pci_devs[i].prog_if == 0x30) {
        usb_debug("usb: XHCI not yet supported\r\n");
      }
    }
  }

  if (controller_count == 0) {
    usb_debug("usb: no controllers found\r\n");
    return -1;
  }

  /* Enumerate devices on each controller */
  for (int i = 0; i < controller_count; ++i) {
    usb_enumerate_controller(controllers[i]);
  }

  usb_debug("usb: init complete, ");
  usb_debug_hex(device_count);
  usb_debug(" devices found\r\n");

  if (usb_msc_ready) {
    brights_usb_msc_register_block();
  }

  return 0;
}

int brights_usb_device_count(void)
{
  return device_count;
}

usb_device_t *brights_usb_device_get(int index)
{
  if (index < 0 || index >= device_count) return 0;
  return &devices[index];
}

usb_controller_t *brights_usb_controller_get(int index)
{
  if (index < 0 || index >= controller_count) return 0;
  return controllers[index];
}

int brights_usb_msc_present(void)
{
  return usb_msc_ready ? 1 : 0;
}
