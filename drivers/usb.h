#ifndef BRIGHTS_USB_H
#define BRIGHTS_USB_H

#include <stdint.h>

/* USB device request (SETUP packet) */
typedef struct {
  uint8_t  bmRequestType;
  uint8_t  bRequest;
  uint16_t wValue;
  uint16_t wIndex;
  uint16_t wLength;
} __attribute__((packed)) usb_device_request_t;

/* Standard device descriptor */
typedef struct {
  uint8_t  bLength;
  uint8_t  bDescriptorType;
  uint16_t bcdUSB;
  uint8_t  bDeviceClass;
  uint8_t  bDeviceSubClass;
  uint8_t  bDeviceProtocol;
  uint8_t  bMaxPacketSize0;
  uint16_t idVendor;
  uint16_t idProduct;
  uint16_t bcdDevice;
  uint8_t  iManufacturer;
  uint8_t  iProduct;
  uint8_t  iSerialNumber;
  uint8_t  bNumConfigurations;
} __attribute__((packed)) usb_device_descriptor_t;

/* Configuration descriptor */
typedef struct {
  uint8_t  bLength;
  uint8_t  bDescriptorType;
  uint16_t wTotalLength;
  uint8_t  bNumInterfaces;
  uint8_t  bConfigurationValue;
  uint8_t  iConfiguration;
  uint8_t  bmAttributes;
  uint8_t  bMaxPower;
} __attribute__((packed)) usb_config_descriptor_t;

/* Interface descriptor */
typedef struct {
  uint8_t  bLength;
  uint8_t  bDescriptorType;
  uint8_t  bInterfaceNumber;
  uint8_t  bAlternateSetting;
  uint8_t  bNumEndpoints;
  uint8_t  bInterfaceClass;
  uint8_t  bInterfaceSubClass;
  uint8_t  bInterfaceProtocol;
  uint8_t  iInterface;
} __attribute__((packed)) usb_interface_descriptor_t;

/* Endpoint descriptor */
typedef struct {
  uint8_t  bLength;
  uint8_t  bDescriptorType;
  uint8_t  bEndpointAddress;
  uint8_t  bmAttributes;
  uint16_t wMaxPacketSize;
  uint8_t  bInterval;
} __attribute__((packed)) usb_endpoint_descriptor_t;

/* HID descriptor */
typedef struct {
  uint8_t  bLength;
  uint8_t  bDescriptorType;
  uint16_t bcdHID;
  uint8_t  bCountryCode;
  uint8_t  bNumDescriptors;
  uint8_t  bReportDescriptorType;
  uint16_t wReportDescriptorLength;
} __attribute__((packed)) usb_hid_descriptor_t;

/* USB device state */
typedef struct {
  uint8_t  address;
  uint8_t  speed;
  uint8_t  port;
  uint8_t  hub_port;
  uint32_t controller_idx;
  uint8_t  configured;
  uint8_t  class_code;
  uint8_t  sub_class;
  uint8_t  protocol;
  uint16_t vendor_id;
  uint16_t product_id;
  uint8_t  max_packet_size;
  uint8_t  num_configs;
  uint8_t  num_interfaces;
  uint8_t  interface_class[8];
  uint8_t  interface_subclass[8];
  uint8_t  interface_protocol[8];
  uint8_t  ep_in_addr[8];
  uint8_t  ep_out_addr[8];
  uint16_t ep_max_packet[8];
  uint8_t  ep_type[8];
  uint8_t  ep_count;
} usb_device_t;

/* USB controller operations (virtual table) */
typedef struct usb_controller usb_controller_t;

typedef int (*usb_ctrl_transfer_t)(usb_controller_t *ctrl, uint8_t dev_addr,
  uint8_t ep, usb_device_request_t *req, void *data);
typedef int (*usb_bulk_transfer_t)(usb_controller_t *ctrl, uint8_t dev_addr,
  uint8_t ep, void *data, uint32_t len, int dir_in);
typedef int (*usb_int_transfer_t)(usb_controller_t *ctrl, uint8_t dev_addr,
  uint8_t ep, void *data, uint32_t len, int dir_in);

struct usb_controller {
  uint32_t index;
  uint8_t  type;
  uint16_t vendor_id;
  uint16_t device_id;
  uint8_t  prog_if;
  uint32_t iobase;
  uint32_t mmio_base;
  uint32_t irq;
  usb_ctrl_transfer_t ctrl_transfer;
  usb_bulk_transfer_t  bulk_transfer;
  usb_int_transfer_t   int_transfer;
};

/* USB controller types */
#define USB_CONTROLLER_UHCI 1
#define USB_CONTROLLER_OHCI 2
#define USB_CONTROLLER_EHCI 3
#define USB_CONTROLLER_XHCI 4

/* USB speeds */
#define USB_SPEED_LOW    0
#define USB_SPEED_FULL   1
#define USB_SPEED_HIGH   2

/* Request type */
#define USB_REQ_TYPE_HOST_TO_DEVICE    (0 << 7)
#define USB_REQ_TYPE_DEVICE_TO_HOST    (1 << 7)
#define USB_REQ_TYPE_STANDARD          (0 << 5)
#define USB_REQ_TYPE_CLASS             (1 << 5)
#define USB_REQ_TYPE_VENDOR            (2 << 5)
#define USB_REQ_RECIPIENT_DEVICE       (0)
#define USB_REQ_RECIPIENT_INTERFACE    (1)
#define USB_REQ_RECIPIENT_ENDPOINT     (2)
#define USB_REQ_RECIPIENT_OTHER        (3)

/* Standard requests */
#define USB_REQ_GET_STATUS            0
#define USB_REQ_CLEAR_FEATURE         1
#define USB_REQ_SET_FEATURE           3
#define USB_REQ_SET_ADDRESS           5
#define USB_REQ_GET_DESCRIPTOR        6
#define USB_REQ_SET_DESCRIPTOR        7
#define USB_REQ_GET_CONFIGURATION     8
#define USB_REQ_SET_CONFIGURATION     9
#define USB_REQ_GET_INTERFACE         10
#define USB_REQ_SET_INTERFACE         11
#define USB_REQ_SYNCH_FRAME           12

/* Descriptor types */
#define USB_DESC_DEVICE               1
#define USB_DESC_CONFIGURATION        2
#define USB_DESC_STRING               3
#define USB_DESC_INTERFACE            4
#define USB_DESC_ENDPOINT             5
#define USB_DESC_HID                  0x21
#define USB_DESC_HID_REPORT           0x22

/* Class codes */
#define USB_CLASS_PER_INTERFACE       0x00
#define USB_CLASS_HID                 0x03
#define USB_CLASS_MASS_STORAGE        0x08
#define USB_CLASS_HUB                 0x09

/* Endpoint types */
#define USB_EP_CONTROL                0
#define USB_EP_ISOCHRONOUS            1
#define USB_EP_BULK                   2
#define USB_EP_INTERRUPT              3

/* Endpoint direction */
#define USB_EP_DIR_OUT                0
#define USB_EP_DIR_IN                 1

/* PID types */
#define USB_PID_OUT                   0xE1
#define USB_PID_IN                    0x69
#define USB_PID_SETUP                 0x2D
#define USB_PID_ACK                   0xD2
#define USB_PID_NAK                   0x5A
#define USB_PID_STALL                 0x1E

/* Hub feature selectors */
#define USB_FEATURE_PORT_RESET        4
#define USB_FEATURE_PORT_POWER        8

/* API functions */
int brights_usb_init(void);
int brights_usb_device_count(void);
usb_device_t *brights_usb_device_get(int index);
usb_controller_t *brights_usb_controller_get(int index);
int brights_usb_hid_read_key(uint8_t *key);
void brights_usb_hid_poll_all(void);
int brights_usb_hid_poll_keyboard(usb_controller_t *ctrl, usb_device_t *dev);
int brights_usb_msc_init(usb_controller_t *ctrl, usb_device_t *dev);
int brights_usb_msc_register_block(void);
int brights_usb_msc_read(uint64_t lba, void *buf, uint32_t count);
int brights_usb_msc_write(uint64_t lba, const void *buf, uint32_t count);
int brights_usb_msc_present(void);

#endif
