#include "trap.h"
#include "trap_console.h"
#include "syscall_abi.h"
#include "pic.h"
#include "../../kernel/core/printf.h"
#include "../../kernel/core/clock.h"
#include "../../kernel/core/sched.h"
#include "../../kernel/core/proc.h"
#include "../../kernel/core/pf.h"
#include "../../drivers/serial.h"
#include "../../drivers/ps2kbd.h"

static brights_console_t trap_console;
static int trap_console_ready;

void brights_trap_console_init(void)
{
  if (!trap_console_ready)
  {
    brights_serial_console_init(&trap_console, BRIGHTS_COM1_PORT);
    trap_console_ready = 1;
  }
}

static uint32_t read_cr2(void)
{
  uint32_t val;
  __asm__ __volatile__("mov %%cr2, %0" : "=r"(val));
  return val;
}

void brights_trap_handler(brights_trap_frame_t *tf)
{
  brights_sched_set_trap_frame(tf);

  if (tf->vec == 0x80)
  {
    tf->eax = (uint32_t)brights_syscall_handle(tf);
    return;
  }

  if (tf->vec == 32)
  {
    brights_clock_tick();
    brights_sched_tick();
    brights_pic_eoi(0);
    return;
  }

  if (tf->vec == 33)
  {
    brights_ps2kbd_irq_handler();
    brights_pic_eoi(1);
    return;
  }

  brights_trap_console_init();

  if (tf->vec == 14)
  {
    uint32_t cr2 = read_cr2();
    if (brights_page_fault_handler(cr2, tf->err) == 0)
    {
      return;
    }
    brights_print(&trap_console, u"BrightS fatal page fault\r\n  cr2=0x");
    for (int i = 7; i >= 0; --i)
    {
      uint16_t hex[] = u"0123456789ABCDEF";
      uint16_t ch[2] = {hex[(cr2 >> (i * 4)) & 0xF], 0};
      brights_print(&trap_console, ch);
    }
    brights_print(&trap_console, u" err=");
    for (int i = 7; i >= 0; --i)
    {
      uint16_t hex[] = u"0123456789ABCDEF";
      uint16_t ch[2] = {hex[(tf->err >> (i * 4)) & 0xF], 0};
      brights_print(&trap_console, ch);
    }
    brights_print(&trap_console, u" eip=");
    for (int i = 7; i >= 0; --i)
    {
      uint16_t hex[] = u"0123456789ABCDEF";
      uint16_t ch[2] = {hex[(tf->eip >> (i * 4)) & 0xF], 0};
      brights_print(&trap_console, ch);
    }
    brights_print(&trap_console, u"\r\n");
    for (;;) { __asm__ __volatile__("hlt"); }
  }

  brights_print(&trap_console, u"BrightS trap\r\n  vec=");
  for (int i = 7; i >= 0; --i)
  {
    uint16_t hex[] = u"0123456789ABCDEF";
    uint16_t ch[2] = {hex[(tf->vec >> (i * 4)) & 0xF], 0};
    brights_print(&trap_console, ch);
  }
  brights_print(&trap_console, u" err=");
  for (int i = 7; i >= 0; --i)
  {
    uint16_t hex[] = u"0123456789ABCDEF";
    uint16_t ch[2] = {hex[(tf->err >> (i * 4)) & 0xF], 0};
    brights_print(&trap_console, ch);
  }
  brights_print(&trap_console, u" eip=");
  for (int i = 7; i >= 0; --i)
  {
    uint16_t hex[] = u"0123456789ABCDEF";
    uint16_t ch[2] = {hex[(tf->eip >> (i * 4)) & 0xF], 0};
    brights_print(&trap_console, ch);
  }
  brights_print(&trap_console, u"\r\n");
  for (;;) { __asm__ __volatile__("hlt"); }
}
