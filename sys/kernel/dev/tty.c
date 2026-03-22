#include "tty.h"
#include "serial.h"
#include "ps2kbd.h"
#include "../core/printf.h"

static brights_console_t tty_console;
static int tty_ready;

void brights_tty_init(void)
{
  if (!tty_ready) {
    brights_serial_console_init(&tty_console, BRIGHTS_COM1_PORT);
    tty_ready = 1;
  }
}

void brights_tty_write(const uint16_t *s)
{
  if (!s) {
    return;
  }
  brights_tty_init();
  brights_print(&tty_console, s);
}

int brights_tty_read_char(char *out_ch)
{
  uint8_t serial_ch = 0;
  if (!out_ch) {
    return -1;
  }
  if (brights_ps2kbd_read_char(out_ch) > 0) {
    return 1;
  }
  if (brights_serial_read_byte(BRIGHTS_COM1_PORT, &serial_ch) > 0) {
    *out_ch = (char)serial_ch;
    return 1;
  }
  return 0;
}

char brights_tty_read_char_blocking(void)
{
  char ch = 0;
  while (brights_tty_read_char(&ch) <= 0) {
  }
  return ch;
}
