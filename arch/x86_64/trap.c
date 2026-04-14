#include "trap.h"
#include "trap_console.h"
#include "syscall_abi.h"
#include "pic.h"
#include "../../kernel/printf.h"
#include "../../kernel/clock.h"
#include "../../kernel/sched.h"
#include "../../kernel/proc.h"
#include "../../kernel/pf.h"
#include "../../drivers/serial.h"
#include "../../drivers/ps2kbd.h"

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

/* Timer tick counter for scheduling quantum */
static uint32_t sched_quantum = 0;
#define SCHED_QUANTUM_TICKS 10  /* Reschedule every 10 timer ticks (100ms at 100Hz) */

void brights_trap_handler(brights_trap_frame_t *tf)
{
  /* Save trap frame for scheduler */
  brights_sched_set_trap_frame(tf);

  /* Syscall via int 0x80 */
  if (tf->vec == 0x80) {
    tf->rax = (uint64_t)brights_syscall_handle(tf);
    return;
  }

  /* Timer interrupt (IRQ0 → vector 32) */
  if (tf->vec == 32) {
    // brights_clock_tick();
    // brights_sched_tick();
    brights_pic_eoi(0);

    /* Preemptive scheduling: every N ticks, yield */
    // ++sched_quantum;
    // if (sched_quantum >= SCHED_QUANTUM_TICKS) {
    //   sched_quantum = 0;
    //   uint32_t cur = brights_sched_current_pid();
    //   if (cur > 0) {
    //     brights_sched_yield();
    //   }
    // }
    return;
  }

  /* Keyboard interrupt (IRQ1 → vector 33) */
  if (tf->vec == 33) {
    brights_ps2kbd_irq_handler();
    brights_pic_eoi(1);
    return;
  }

  /* Unexpected trap */
  brights_trap_console_init();

  /* Page fault (vector 14) */
  if (tf->vec == 14) {
    uint64_t cr2 = read_cr2();
    if (brights_page_fault_handler(cr2, tf->err) == 0) {
      /* Page fault handled successfully */
      return;
    }
    /* Fatal page fault */
    brights_print(&trap_console, u"BrightS fatal page fault\r\n  cr2=");
    trap_print_hex(cr2);
    brights_print(&trap_console, u" err=");
    trap_print_hex(tf->err);
    brights_print(&trap_console, u" rip=");
    trap_print_hex(tf->rip);
    brights_print(&trap_console, u"\r\n");
    for (;;) {
      __asm__ __volatile__("hlt");
    }
  }

  brights_print(&trap_console, u"BrightS trap\r\n  vec=");
  trap_print_hex(tf->vec);
  brights_print(&trap_console, u" err=");
  trap_print_hex(tf->err);
  brights_print(&trap_console, u" rip=");
  trap_print_hex(tf->rip);
  brights_print(&trap_console, u"\r\n");
  for (;;) {
    __asm__ __volatile__("hlt");
  }
}
