#include "ps2kbd.h"
#include "../platform/x86_64/io.h"
#include <stdint.h>

#define PS2_DATA_PORT   0x60
#define PS2_STATUS_PORT 0x64

static int ps2_shift;
static int ps2_caps_lock;
static int ps2_extended;

static char letter_with_case(char base)
{
  if ((ps2_shift ^ ps2_caps_lock) != 0) {
    return (char)(base - ('a' - 'A'));
  }
  return base;
}

static int ps2_scancode_to_ascii(uint8_t sc, char *out)
{
  switch (sc) {
    case 0x02: *out = ps2_shift ? '!' : '1'; return 1;
    case 0x03: *out = ps2_shift ? '@' : '2'; return 1;
    case 0x04: *out = ps2_shift ? '#' : '3'; return 1;
    case 0x05: *out = ps2_shift ? '$' : '4'; return 1;
    case 0x06: *out = ps2_shift ? '%' : '5'; return 1;
    case 0x07: *out = ps2_shift ? '^' : '6'; return 1;
    case 0x08: *out = ps2_shift ? '&' : '7'; return 1;
    case 0x09: *out = ps2_shift ? '*' : '8'; return 1;
    case 0x0A: *out = ps2_shift ? '(' : '9'; return 1;
    case 0x0B: *out = ps2_shift ? ')' : '0'; return 1;
    case 0x0C: *out = ps2_shift ? '_' : '-'; return 1;
    case 0x0D: *out = ps2_shift ? '+' : '='; return 1;
    case 0x10: *out = letter_with_case('q'); return 1;
    case 0x11: *out = letter_with_case('w'); return 1;
    case 0x12: *out = letter_with_case('e'); return 1;
    case 0x13: *out = letter_with_case('r'); return 1;
    case 0x14: *out = letter_with_case('t'); return 1;
    case 0x15: *out = letter_with_case('y'); return 1;
    case 0x16: *out = letter_with_case('u'); return 1;
    case 0x17: *out = letter_with_case('i'); return 1;
    case 0x18: *out = letter_with_case('o'); return 1;
    case 0x19: *out = letter_with_case('p'); return 1;
    case 0x1A: *out = ps2_shift ? '{' : '['; return 1;
    case 0x1B: *out = ps2_shift ? '}' : ']'; return 1;
    case 0x1C: *out = '\n'; return 1;
    case 0x1E: *out = letter_with_case('a'); return 1;
    case 0x1F: *out = letter_with_case('s'); return 1;
    case 0x20: *out = letter_with_case('d'); return 1;
    case 0x21: *out = letter_with_case('f'); return 1;
    case 0x22: *out = letter_with_case('g'); return 1;
    case 0x23: *out = letter_with_case('h'); return 1;
    case 0x24: *out = letter_with_case('j'); return 1;
    case 0x25: *out = letter_with_case('k'); return 1;
    case 0x26: *out = letter_with_case('l'); return 1;
    case 0x27: *out = ps2_shift ? ':' : ';'; return 1;
    case 0x28: *out = ps2_shift ? '"' : '\''; return 1;
    case 0x29: *out = ps2_shift ? '~' : '`'; return 1;
    case 0x2B: *out = ps2_shift ? '|' : '\\'; return 1;
    case 0x2C: *out = letter_with_case('z'); return 1;
    case 0x2D: *out = letter_with_case('x'); return 1;
    case 0x2E: *out = letter_with_case('c'); return 1;
    case 0x2F: *out = letter_with_case('v'); return 1;
    case 0x30: *out = letter_with_case('b'); return 1;
    case 0x31: *out = letter_with_case('n'); return 1;
    case 0x32: *out = letter_with_case('m'); return 1;
    case 0x33: *out = ps2_shift ? '<' : ','; return 1;
    case 0x34: *out = ps2_shift ? '>' : '.'; return 1;
    case 0x35: *out = ps2_shift ? '?' : '/'; return 1;
    case 0x39: *out = ' '; return 1;
    case 0x0E: *out = '\b'; return 1;
    case 0x0F: *out = '\t'; return 1;
    case 0x01: *out = 27; return 1;
    default: return 0;
  }
}

int brights_ps2kbd_init(void)
{
  ps2_shift = 0;
  ps2_caps_lock = 0;
  ps2_extended = 0;
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
  if (sc == 0xE0u) {
    ps2_extended = 1;
    return 0;
  }

  if (sc == 0x2Au || sc == 0x36u) {
    ps2_shift = 1;
    return 0;
  }
  if (sc == 0xAAu || sc == 0xB6u) {
    ps2_shift = 0;
    return 0;
  }
  if (sc == 0x3Au) {
    ps2_caps_lock = !ps2_caps_lock;
    return 0;
  }

  if (ps2_extended) {
    ps2_extended = 0;
    switch (sc) {
      case 0x53: *out_ch = 0x7F; return 1;
      default: return 0;
    }
  }

  if (sc & 0x80u) {
    return 0;
  }
  return ps2_scancode_to_ascii(sc, out_ch);
}
