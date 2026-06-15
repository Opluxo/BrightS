#include "idt.h"
#include "pic.h"
#include "pit.h"
#include "../../kernel/core/printf.h"
#include "../../drivers/serial.h"

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
extern void brights_isr_stub_1(void);
extern void brights_isr_stub_2(void);
extern void brights_isr_stub_3(void);
extern void brights_isr_stub_4(void);
extern void brights_isr_stub_5(void);
extern void brights_isr_stub_6(void);
extern void brights_isr_stub_7(void);
extern void brights_isr_stub_8(void);
extern void brights_isr_stub_9(void);
extern void brights_isr_stub_10(void);
extern void brights_isr_stub_11(void);
extern void brights_isr_stub_12(void);
extern void brights_isr_stub_13(void);
extern void brights_isr_stub_14(void);
extern void brights_isr_stub_15(void);
extern void brights_isr_stub_16(void);
extern void brights_isr_stub_17(void);
extern void brights_isr_stub_18(void);
extern void brights_isr_stub_19(void);
extern void brights_isr_stub_20(void);
extern void brights_isr_stub_21(void);
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

  /* Set default stub for unregistered vectors */
  for (int i = 0; i < 256; ++i) {
    idt_set_gate(i, brights_isr_stub_0, 0x8E);
  }

  /* CPU Exceptions 0-21 (Intel reserved) */
  idt_set_gate(0,  brights_isr_stub_0,  0x8E);  /* #DE */
  idt_set_gate(1,  brights_isr_stub_1,  0x8E);  /* #DB */
  idt_set_gate(2,  brights_isr_stub_2,  0x8E);  /* NMI */
  idt_set_gate(3,  brights_isr_stub_3,  0x8E);  /* #BP */
  idt_set_gate(4,  brights_isr_stub_4,  0x8E);  /* #OF */
  idt_set_gate(5,  brights_isr_stub_5,  0x8E);  /* #BR */
  idt_set_gate(6,  brights_isr_stub_6,  0x8E);  /* #UD */
  idt_set_gate(7,  brights_isr_stub_7,  0x8E);  /* #NM */
  idt_set_gate(8,  brights_isr_stub_8,  0x8E);  /* #DF */
  idt_set_gate(9,  brights_isr_stub_9,  0x8E);  /* reserved */
  idt_set_gate(10, brights_isr_stub_10, 0x8E);  /* #TS */
  idt_set_gate(11, brights_isr_stub_11, 0x8E);  /* #NP */
  idt_set_gate(12, brights_isr_stub_12, 0x8E);  /* #SS */
  idt_set_gate(13, brights_isr_stub_13, 0x8E);  /* #GP */
  idt_set_gate(14, brights_isr_stub_14, 0x8E);  /* #PF */
  idt_set_gate(15, brights_isr_stub_15, 0x8E);  /* reserved */
  idt_set_gate(16, brights_isr_stub_16, 0x8E);  /* #MF */
  idt_set_gate(17, brights_isr_stub_17, 0x8E);  /* #AC */
  idt_set_gate(18, brights_isr_stub_18, 0x8E);  /* #MC */
  idt_set_gate(19, brights_isr_stub_19, 0x8E);  /* #XM */
  idt_set_gate(20, brights_isr_stub_20, 0x8E);  /* #VE */
  idt_set_gate(21, brights_isr_stub_21, 0x8E);  /* reserved */

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

  /* Enable interrupts - temporarily disabled for debugging */
  // __asm__ __volatile__("sti");
}
