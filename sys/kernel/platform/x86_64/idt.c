#include "idt.h"

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
extern void brights_isr_stub_128(void);

static void idt_set_gate(int vec, void (*isr)(void), uint8_t type_attr)
{
  uint64_t addr = (uint64_t)isr;
  idt[vec].offset_low = addr & 0xFFFF;
  idt[vec].selector = 0x08; // kernel code segment
  idt[vec].ist = 0;
  idt[vec].type_attr = type_attr;
  idt[vec].offset_mid = (addr >> 16) & 0xFFFF;
  idt[vec].offset_high = (addr >> 32) & 0xFFFFFFFF;
  idt[vec].zero = 0;
}

void brights_idt_init(void)
{
  for (int i = 0; i < 256; ++i) {
    idt_set_gate(i, brights_isr_stub_0, 0x8E);
  }
  idt_set_gate(13, brights_isr_stub_13, 0x8E);
  idt_set_gate(14, brights_isr_stub_14, 0x8E);
  idt_set_gate(0x80, brights_isr_stub_128, 0xEE);

  struct idt_ptr idtr;
  idtr.limit = sizeof(idt) - 1;
  idtr.base = (uint64_t)&idt[0];
  __asm__ __volatile__("lidt %0" : : "m"(idtr));
}
