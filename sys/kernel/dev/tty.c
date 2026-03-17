#include "tty.h"
#include "serial.h"
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
