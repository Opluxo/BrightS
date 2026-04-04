#include "idt.h"
#include "pic.h"
#include "pit.h"
#include "../../core/printf.h"
#include "../../dev/serial.h"

struct idt_entry {
  uint16_t offset_low;
  uint16_t selector;
  uint8_t ist;
  uint8_t type_attr;
  uint16_t offset_mid;
  uint32_t offset_high;
  uint32_t zero;
} __attribute__((packed));

struct idt_ptr {
  uint16_t limit;
  uint64_t base;
} __attribute__((packed));

static struct idt_entry idt[256];

extern void brights_isr_stub_0(void);
extern void brights_isr_stub_13(void);
extern void brights_isr_stub_14(void);
extern void brights_isr_stub_32(void);
extern void brights_isr_stub_33(void);
extern void brights_isr_stub_128(void);

static void idt_set_gate(int vec, void (*isr)(void), uint8_t type_attr)
{
  uint64_t addr = (uint64_t)isr;
  idt[vec].offset_low = addr & 0xFFFF;
  idt[vec].selector = 0x08;
  idt[vec].ist = 0;
  idt[vec].type_attr = type_attr;
  idt[vec].offset_mid = (addr >> 16) & 0xFFFF;
  idt[vec].offset_high = (addr >> 32) & 0xFFFFFFFF;
  idt[vec].zero = 0;
}

void brights_idt_init(void)
{
  /* Remap PIC first */
  brights_pic_remap();

  /* Set all vectors to stub_0 initially */
  for (int i = 0; i < 256; ++i) {
    idt_set_gate(i, brights_isr_stub_0, 0x8E);
  }

  /* CPU exceptions */
  idt_set_gate(13, brights_isr_stub_13, 0x8E);
  idt_set_gate(14, brights_isr_stub_14, 0x8E);

  /* Hardware IRQs */
  idt_set_gate(32, brights_isr_stub_32, 0x8E); /* PIT timer (IRQ0) */
  idt_set_gate(33, brights_isr_stub_33, 0x8E); /* Keyboard (IRQ1) */

  /* Syscall */
  idt_set_gate(0x80, brights_isr_stub_128, 0xEE);

  /* Load IDT */
  struct idt_ptr idtr;
  idtr.limit = sizeof(idt) - 1;
  idtr.base = (uint64_t)&idt[0];
  __asm__ __volatile__("lidt %0" : : "m"(idtr));

  /* Unmask timer and keyboard IRQs */
  brights_pic_unmask(0); /* IRQ0 - PIT */
  brights_pic_unmask(1); /* IRQ1 - Keyboard */
  /* Mask IRQ2 (cascade) and others */
  for (int i = 2; i < 16; ++i) {
    brights_pic_mask((uint8_t)i);
  }

  /* Initialize PIT at 100 Hz */
  brights_pit_init(100);

  /* Enable interrupts */
  __asm__ __volatile__("sti");
}
