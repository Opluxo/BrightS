#include "pic.h"
#include "pit.h"
#include "io.h"

/* ===== PIC ===== */

#define PIC1_CMD   0x20
#define PIC1_DATA  0x21
#define PIC2_CMD   0xA0
#define PIC2_DATA  0xA1
#define ICW1_INIT  0x11
#define ICW4_8086  0x01

void brights_pic_remap(void)
{
  uint8_t m1 = inb(PIC1_DATA);
  uint8_t m2 = inb(PIC2_DATA);

  outb(PIC1_CMD, ICW1_INIT);
  outb(PIC2_CMD, ICW1_INIT);
  outb(PIC1_DATA, 0x20);
  outb(PIC2_DATA, 0x28);
  outb(PIC1_DATA, 0x04);
  outb(PIC2_DATA, 0x02);
  outb(PIC1_DATA, ICW4_8086);
  outb(PIC2_DATA, ICW4_8086);
  outb(PIC1_DATA, m1);
  outb(PIC2_DATA, m2);
}

void brights_pic_eoi(uint8_t irq)
{
  if (irq >= 8) outb(PIC2_CMD, 0x20);
  outb(PIC1_CMD, 0x20);
}

void brights_pic_mask(uint8_t irq)
{
  uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
  if (irq >= 8) irq -= 8;
  outb(port, inb(port) | (1u << irq));
}

void brights_pic_unmask(uint8_t irq)
{
  uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
  if (irq >= 8) irq -= 8;
  outb(port, inb(port) & ~(1u << irq));
}

/* ===== PIT ===== */

#define PIT_CH0   0x40
#define PIT_CMD   0x43
#define PIT_FREQ  1193182ULL

void brights_pit_init(uint32_t frequency_hz)
{
  if (frequency_hz == 0) frequency_hz = 100;
  uint16_t divisor = (uint16_t)(PIT_FREQ / frequency_hz);

  outb(PIT_CMD, 0x36);
  outb(PIT_CH0, (uint8_t)(divisor & 0xFF));
  outb(PIT_CH0, (uint8_t)((divisor >> 8) & 0xFF));
}
