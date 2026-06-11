#include "idt.h"
#include "gdt.h"
#include "io.h"
#include "pic.h"

struct idt_entry
{
  uint16_t offset_low;
  uint16_t selector;
  uint8_t zero;
  uint8_t type_attr;
  uint16_t offset_high;
} __attribute__((packed));

struct idt_ptr
{
  uint16_t limit;
  uint32_t base;
} __attribute__((packed));

static struct idt_entry idt[256];

extern void brights_isr_stub_default(void);
extern void brights_isr_stub_13(void);
extern void brights_isr_stub_14(void);
extern void brights_isr_stub_32(void);
extern void brights_isr_stub_33(void);
extern void brights_isr_stub_128(void);

static void idt_set_gate(int vec, void (*isr)(void), uint8_t type_attr)
{
  uint32_t addr = (uint32_t)isr;
  idt[vec].offset_low = addr & 0xFFFF;
  idt[vec].selector = BRIGHTS_KERNEL_CS;
  idt[vec].zero = 0;
  idt[vec].type_attr = type_attr;
  idt[vec].offset_high = (addr >> 16) & 0xFFFF;
}

void brights_idt_init(void)
{
  for (int i = 0; i < 256; ++i)
  {
    idt_set_gate(i, brights_isr_stub_default, 0x8E);
  }

  idt_set_gate(13, brights_isr_stub_13, 0x8E);
  idt_set_gate(14, brights_isr_stub_14, 0x8E);
  idt_set_gate(32, brights_isr_stub_32, 0x8E);
  idt_set_gate(33, brights_isr_stub_33, 0x8E);
  idt_set_gate(0x80, brights_isr_stub_128, 0xEE);

  struct idt_ptr idtr;
  idtr.limit = sizeof(idt) - 1;
  idtr.base = (uint32_t)&idt[0];
  __asm__ __volatile__("lidt %0" : : "m"(idtr));

  brights_pic_unmask(0);
  brights_pic_unmask(1);
}
