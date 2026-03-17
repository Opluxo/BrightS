#include "ps2kbd.h"
#include "../platform/x86_64/io.h"
#include <stdint.h>

#define PS2_DATA_PORT   0x60
#define PS2_STATUS_PORT 0x64

static int ps2_scancode_to_ascii(uint8_t sc, char *out)
{
  // Set-1 make codes only.
  switch (sc) {
    case 0x02: *out = '1'; return 1;
    case 0x03: *out = '2'; return 1;
    case 0x04: *out = '3'; return 1;
    case 0x05: *out = '4'; return 1;
    case 0x06: *out = '5'; return 1;
    case 0x07: *out = '6'; return 1;
    case 0x08: *out = '7'; return 1;
    case 0x09: *out = '8'; return 1;
    case 0x0A: *out = '9'; return 1;
    case 0x0B: *out = '0'; return 1;
    case 0x10: *out = 'q'; return 1;
    case 0x11: *out = 'w'; return 1;
    case 0x12: *out = 'e'; return 1;
    case 0x13: *out = 'r'; return 1;
    case 0x14: *out = 't'; return 1;
    case 0x15: *out = 'y'; return 1;
    case 0x16: *out = 'u'; return 1;
    case 0x17: *out = 'i'; return 1;
    case 0x18: *out = 'o'; return 1;
    case 0x19: *out = 'p'; return 1;
    case 0x1E: *out = 'a'; return 1;
    case 0x1F: *out = 's'; return 1;
    case 0x20: *out = 'd'; return 1;
    case 0x21: *out = 'f'; return 1;
    case 0x22: *out = 'g'; return 1;
    case 0x23: *out = 'h'; return 1;
    case 0x24: *out = 'j'; return 1;
    case 0x25: *out = 'k'; return 1;
    case 0x26: *out = 'l'; return 1;
    case 0x2C: *out = 'z'; return 1;
    case 0x2D: *out = 'x'; return 1;
    case 0x2E: *out = 'c'; return 1;
    case 0x2F: *out = 'v'; return 1;
    case 0x30: *out = 'b'; return 1;
    case 0x31: *out = 'n'; return 1;
    case 0x32: *out = 'm'; return 1;
    case 0x39: *out = ' '; return 1;
    case 0x1C: *out = '\n'; return 1;
    case 0x0E: *out = '\b'; return 1;
    case 0x01: *out = 27; return 1; // ESC
    default: return 0;
  }
}

int brights_ps2kbd_init(void)
{
  // Drain any stale bytes in output buffer.
  for (int i = 0; i < 32; ++i) {
    if ((inb(PS2_STATUS_PORT) & 0x01u) == 0) {
      break;
    }
    (void)inb(PS2_DATA_PORT);
  }
  return 0;
}

int brights_ps2kbd_read_char(char *out_ch)
{
  if (!out_ch) {
    return -1;
  }
  if ((inb(PS2_STATUS_PORT) & 0x01u) == 0) {
    return 0;
  }
  uint8_t sc = inb(PS2_DATA_PORT);
  if (sc & 0x80u) {
    return 0; // break code
  }
  return ps2_scancode_to_ascii(sc, out_ch);
}
