#include "idt.h"
#include "gdt.h"
#include "io.h"

#define PIC1        0x20
#define PIC2        0xA0
#define PIC1_CMD    PIC1
#define PIC1_DATA   (PIC1 + 1)
#define PIC2_CMD    PIC2
#define PIC2_DATA   (PIC2 + 1)

#define ICW1_ICW4   0x01
#define ICW1_INIT   0x10

void brights_pic_remap(void)
{
  uint8_t m1 = inb(PIC1_DATA);
  uint8_t m2 = inb(PIC2_DATA);

  outb(PIC1_CMD, ICW1_INIT | ICW1_ICW4);
  io_wait();
  outb(PIC2_CMD, ICW1_INIT | ICW1_ICW4);
  io_wait();

  outb(PIC1_DATA, 0x20);
  io_wait();
  outb(PIC2_DATA, 0x28);
  io_wait();

  outb(PIC1_DATA, 4);
  io_wait();
  outb(PIC2_DATA, 2);
  io_wait();

  outb(PIC1_DATA, 0x01);
  io_wait();
  outb(PIC2_DATA, 0x01);
  io_wait();

  outb(PIC1_DATA, m1);
  outb(PIC2_DATA, m2);
}

void brights_pic_mask(uint8_t irq)
{
  uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
  if (irq >= 8) irq -= 8;
  outb(port, inb(port) | (1 << irq));
}

void brights_pic_unmask(uint8_t irq)
{
  uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
  if (irq >= 8) irq -= 8;
  outb(port, inb(port) & ~(1 << irq));
}

void brights_pic_eoi(uint8_t irq)
{
  if (irq >= 8)
  {
    outb(PIC2_CMD, 0x20);
  }
  outb(PIC1_CMD, 0x20);
}

void brights_pit_init(uint32_t freq_hz)
{
  uint32_t divisor = 1193182 / freq_hz;
  outb(0x43, 0x36);
  outb(0x40, divisor & 0xFF);
  outb(0x40, (divisor >> 8) & 0xFF);
}
