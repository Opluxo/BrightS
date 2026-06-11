#include "usb.h"
#ifdef __i386__
#include "../arch/i386/io.h"
#else
#include "../arch/x86_64/io.h"
#endif
#include "../include/kernel/kmalloc.h"
#include "../kernel/core/printf.h"
#include "serial.h"

/* USB HID keyboard report (boot protocol) */
typedef struct {
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t keys[6];
} __attribute__((packed)) hid_keyboard_report_t;

/* USB HID keycode to ASCII mapping (US layout) */
static const uint8_t usb_hid_keymap[256] = {
  0,   0,   0,   0,   0,   0,   0,   0,    /* 0x00-0x07 */
 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B,  /* 0x08-0x0F */
  0,   0,   0,   0,   'a', 'b', 'c', 'd',  /* 0x10-0x17 */
  'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',  /* 0x18-0x1F */
  'm', 'n', 'o', 'p', 'q', 'r', 's', 't',  /* 0x20-0x27 */
  'u', 'v', 'w', 'x', 'y', 'z',            /* 0x28-0x2D */
  '1', '2', '3', '4', '5', '6', '7', '8',  /* 0x2E-0x35 */
  '9', '0', '\n', 0x1B, 0x08, '\t', ' ',   /* 0x36-0x3C */
  '-', '=', '[', ']', '\\',                 /* 0x3D-0x41 */
  0,    ';', '\'', 0x60, ',', '.', '/',     /* 0x42-0x48 */
  0,   0,   0,   0,   0,   0,   0,   0,   0, 0, 0, 0, /* 0x49-0x54 */
  0,   0,   0,   0,   0,   0,   0,   0,   0, 0, 0,     /* keypad */
  0,   0,   0,   0,   0,   0,   0,   0,   0,           /* 0x60-0x68 */
};

/* Shifted keymap */
static const uint8_t usb_hid_keymap_shifted[256] = {
  0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   'A', 'B', 'C', 'D',
  'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
  'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
  'U', 'V', 'W', 'X', 'Y', 'Z',
  '!', '@', '#', '$', '%', '^', '&', '*',
  '(', ')', '\n', 0x1B, 0x08, '\t', ' ',
  '_', '+', '{', '}', '|',
  0,    ':', '"', '~', '<', '>', '?',
};

static int hid_key_head = 0;
static int hid_key_tail = 0;
static uint8_t hid_key_buffer[256];

static void hid_keyboard_enqueue(uint8_t key)
{
  int next = (hid_key_head + 1) % 256;
  if (next != hid_key_tail) {
    hid_key_buffer[hid_key_head] = key;
    hid_key_head = next;
  }
}

/* Read a key from the USB HID keyboard buffer */
int brights_usb_hid_read_key(uint8_t *key)
{
  if (!key) return -1;
  if (hid_key_head == hid_key_tail) return -1;
  *key = hid_key_buffer[hid_key_tail];
  hid_key_tail = (hid_key_tail + 1) % 256;
  return 0;
}

/* Poll a USB HID keyboard for input */
int brights_usb_hid_poll_keyboard(usb_controller_t *ctrl, usb_device_t *dev)
{
  hid_keyboard_report_t report;
  static hid_keyboard_report_t prev_report;
  static int initialized = 0;

  if (!ctrl || !ctrl->int_transfer) return -1;

  /* Find interrupt IN endpoint */
  uint8_t int_in_ep = 0;
  for (int i = 0; i < dev->ep_count; ++i) {
    if (dev->ep_type[i] == USB_EP_INTERRUPT && (dev->ep_in_addr[i] & 0x80)) {
      int_in_ep = dev->ep_in_addr[i];
      break;
    }
  }
  if (int_in_ep == 0) return -1;

  int ret = ctrl->int_transfer(ctrl, dev->address, int_in_ep,
    &report, sizeof(report), 1);

  if (ret < 0) return -1;

  if (!initialized) {
    prev_report = report;
    initialized = 1;
    return 0;
  }

  /* Detect key press: key was not pressed before but is now */
  for (int i = 0; i < 6; ++i) {
    uint8_t key = report.keys[i];
    if (key == 0) continue;

    /* Check if key was NOT in previous report */
    int was_pressed = 0;
    for (int j = 0; j < 6; ++j) {
      if (prev_report.keys[j] == key) { was_pressed = 1; break; }
    }

    if (!was_pressed && key < 0xE0) {
      int shifted = (report.modifiers & 0x22) ? 1 : 0;
      uint8_t ascii = shifted ? usb_hid_keymap_shifted[key] : usb_hid_keymap[key];
      if (ascii) hid_keyboard_enqueue(ascii);
    }
  }

  prev_report = report;
  return 0;
}

void brights_usb_hid_poll_all(void)
{
  int count = brights_usb_device_count();
  for (int i = 0; i < count; ++i) {
    usb_device_t *dev = brights_usb_device_get(i);
    if (!dev || !dev->configured) continue;
    int is_hid = 0;
    for (int j = 0; j < dev->num_interfaces && j < 8; ++j) {
      if (dev->interface_class[j] == USB_CLASS_HID) { is_hid = 1; break; }
    }
    if (!is_hid) continue;
    usb_controller_t *ctrl = brights_usb_controller_get(dev->controller_idx);
    if (!ctrl) continue;
    brights_usb_hid_poll_keyboard(ctrl, dev);
  }
}
