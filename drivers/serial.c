#include "serial.h"
#include "../arch/x86_64/io.h"

static void serial_write_byte(uint16_t port, uint8_t val)
{
  while ((inb(port + 5) & 0x20) == 0) {
  }
  outb(port, val);
}

void brights_serial_init(uint16_t port)
{
  outb(port + 1, 0x00);
  outb(port + 3, 0x80);
  outb(port + 0, 0x03);
  outb(port + 1, 0x00);
  outb(port + 3, 0x03);
  outb(port + 2, 0xC7);
  outb(port + 4, 0x0B);
}

static void serial_puts(brights_console_t *con, const uint16_t *msg)
{
  uint16_t port = (uint16_t)(uintptr_t)con->ctx;
  for (const uint16_t *p = msg; *p != 0; ++p) {
    uint8_t ch = (uint8_t)(*p & 0xFF);
    if (ch == '\n') {
      serial_write_byte(port, '\r');
    }
    serial_write_byte(port, ch);
  }
}

void brights_serial_console_init(brights_console_t *con, uint16_t port)
{
  brights_serial_init(port);
  brights_console_init(con, (void *)(uintptr_t)port, serial_puts);
}

int brights_serial_read_byte(uint16_t port, uint8_t *out)
{
  if (!out) {
    return -1;
  }
  if ((inb(port + 5) & 0x01) == 0) {
    return 0;
  }
  *out = inb(port);
  return 1;
}

uint8_t brights_serial_read_byte_blocking(uint16_t port)
{
  uint8_t ch = 0;
  while (brights_serial_read_byte(port, &ch) <= 0) {
  }
  return ch;
}

void brights_serial_write_ascii(uint16_t port, const char *s)
{
  if (!s) {
    return;
  }
  while (*s) {
    uint8_t ch = (uint8_t)*s++;
    if (ch == '\n') {
      serial_write_byte(port, '\r');
    }
    serial_write_byte(port, ch);
  }
}
