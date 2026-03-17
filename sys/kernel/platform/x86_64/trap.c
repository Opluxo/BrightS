#include "trap.h"
#include "trap_console.h"
#include "syscall_abi.h"
#include "../../core/printf.h"
#include "../../dev/serial.h"

static brights_console_t trap_console;
static int trap_console_ready;

static void trap_print_hex(uint64_t v)
{
  static const uint16_t hex[] = u"0123456789ABCDEF";
  uint16_t buf[2 + 16 + 1];
  buf[0] = '0';
  buf[1] = 'x';
  for (int i = 0; i < 16; ++i) {
    buf[2 + i] = hex[(v >> ((15 - i) * 4)) & 0xF];
  }
  buf[18] = 0;
  brights_print(&trap_console, buf);
}

void brights_trap_console_init(void)
{
  if (!trap_console_ready) {
    brights_serial_console_init(&trap_console, BRIGHTS_COM1_PORT);
    trap_console_ready = 1;
  }
}

static uint64_t read_cr2(void)
{
  uint64_t val;
  __asm__ __volatile__("mov %%cr2, %0" : "=r"(val));
  return val;
}

void brights_trap_handler(brights_trap_frame_t *tf)
{
  if (tf->vec == 0x80) {
    tf->rax = (uint64_t)brights_syscall_handle(tf);
    return;
  }

  brights_trap_console_init();
  brights_print(&trap_console, u"BrightS trap\r\n  vec=");
  trap_print_hex(tf->vec);
  brights_print(&trap_console, u" err=");
  trap_print_hex(tf->err);
  brights_print(&trap_console, u" rip=");
  trap_print_hex(tf->rip);

  if (tf->vec == 14) {
    brights_print(&trap_console, u" cr2=");
    trap_print_hex(read_cr2());
  }

  brights_print(&trap_console, u"\r\n");
  for (;;) {
    __asm__ __volatile__("hlt");
  }
}
