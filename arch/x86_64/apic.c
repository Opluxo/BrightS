#include "apic.h"
#include "msr.h"
#include "io.h"
#include "../../drivers/serial.h"
#include "../../kernel/printf.h"

#define IA32_APIC_BASE_MSR  0x1B
#define IA32_APIC_BSP_BIT   (1ULL << 8)
#define IA32_APIC_ENABLE    (1ULL << 11)

/* APIC MMIO register offsets */
#define APIC_REG_ID         0x020
#define APIC_REG_VER        0x030
#define APIC_REG_TPR        0x080
#define APIC_REG_EOI        0x0B0
#define APIC_REG_SVR        0x0F0
#define APIC_REG_ISR0       0x100
#define APIC_REG_TMR0       0x180
#define APIC_REG_IRR0       0x200
#define APIC_REG_ICR_LO     0x300
#define APIC_REG_ICR_HI     0x310
#define APIC_REG_LVT_TIMER  0x320
#define APIC_REG_LVT_LINT0  0x350
#define APIC_REG_LVT_LINT1  0x360
#define APIC_REG_LVT_ERR    0x370
#define APIC_REG_TIMER_INIT 0x380
#define APIC_REG_TIMER_CUR  0x390
#define APIC_REG_TIMER_DIV  0x3E0

/* SVR bits */
#define APIC_SVR_ENABLE     (1 << 8)
#define APIC_SVR_SPURIOUS   0xFF

/* LVT Timer bits */
#define APIC_TIMER_PERIODIC (1 << 17)
#define APIC_TIMER_MASKED   (1 << 16)

/* Timer divide values */
#define APIC_TIMER_DIV_1    0x0B
#define APIC_TIMER_DIV_2    0x00
#define APIC_TIMER_DIV_4    0x01
#define APIC_TIMER_DIV_8    0x02
#define APIC_TIMER_DIV_16   0x03
#define APIC_TIMER_DIV_32   0x08
#define APIC_TIMER_DIV_64   0x09
#define APIC_TIMER_DIV_128  0x0A

#define APIC_TIMER_VECTOR   0x20  /* Same as our PIT vector 32 */

static volatile uint32_t *apic_base = 0;
static uint32_t apic_timer_freq = 0;
static int apic_ready = 0;

uint32_t brights_apic_read(uint32_t reg)
{
  if (!apic_base) return 0;
  return apic_base[reg / 4];
}

void brights_apic_write(uint32_t reg, uint32_t val)
{
  if (!apic_base) return;
  apic_base[reg / 4] = val;
}

int brights_apic_init(void)
{
  /* Read APIC base from MSR */
  uint64_t apic_base_msr = rdmsr(IA32_APIC_BASE_MSR);
  uint64_t base_addr = apic_base_msr & 0xFFFFF000ULL;

  /* Check if APIC is available */
  if ((apic_base_msr & IA32_APIC_ENABLE) == 0) {
    /* Try to enable it */
    wrmsr(IA32_APIC_BASE_MSR, apic_base_msr | IA32_APIC_ENABLE);
    apic_base_msr = rdmsr(IA32_APIC_BASE_MSR);
    if ((apic_base_msr & IA32_APIC_ENABLE) == 0) return -1;
  }

  apic_base = (volatile uint32_t *)(uintptr_t)base_addr;

  /* Enable APIC: set SVR with enable bit */
  uint32_t svr = APIC_SVR_ENABLE | APIC_SVR_SPURIOUS;
  brights_apic_write(APIC_REG_SVR, svr);

  /* Mask LINT0 and LINT1 (we use IOAPIC for external interrupts) */
  brights_apic_write(APIC_REG_LVT_LINT0, APIC_TIMER_MASKED);
  brights_apic_write(APIC_REG_LVT_LINT1, APIC_TIMER_MASKED);
  brights_apic_write(APIC_REG_LVT_ERR, APIC_TIMER_MASKED);

  /* Set Task Priority to 0 (accept all interrupts) */
  brights_apic_write(APIC_REG_TPR, 0);

  apic_ready = 1;
  return 0;
}

int brights_apic_available(void)
{
  return apic_ready;
}

uint32_t brights_apic_id(void)
{
  if (!apic_ready) return 0;
  return (brights_apic_read(APIC_REG_ID) >> 24) & 0xFF;
}

void brights_apic_eoi(void)
{
  if (apic_base) {
    brights_apic_write(APIC_REG_EOI, 0);
  }
}

void brights_apic_timer_calibrate(void)
{
  if (!apic_ready) return;

  /* Use PIT channel 2 for ~10ms timing */
  outb(0x61, (inb(0x61) & ~0x02) | 0x01);

  /* Set APIC timer: divide by 1, one-shot, masked */
  brights_apic_write(APIC_REG_TIMER_DIV, APIC_TIMER_DIV_1);
  brights_apic_write(APIC_REG_LVT_TIMER, APIC_TIMER_MASKED);

  /* Start PIT one-shot */
  outb(0x43, 0xB0);
  uint16_t pit_count = 11932;
  outb(0x42, pit_count & 0xFF);
  outb(0x42, (pit_count >> 8) & 0xFF);

  uint8_t port61 = inb(0x61);
  outb(0x61, port61 & ~1);
  outb(0x61, port61 | 1);

  /* Start APIC timer with max count */
  brights_apic_write(APIC_REG_TIMER_INIT, 0xFFFFFFFF);
  brights_apic_write(APIC_REG_LVT_TIMER, APIC_TIMER_MASKED); /* one-shot */

  /* Wait for PIT to expire */
  while ((inb(0x61) & 0x20) == 0) { }

  /* Read APIC timer count */
  uint32_t elapsed = 0xFFFFFFFF - brights_apic_read(APIC_REG_TIMER_CUR);

  /* elapsed ticks in ~10ms → freq = elapsed * 100 */
  apic_timer_freq = elapsed * 100;

  /* Sanity check */
  if (apic_timer_freq < 1000000 || apic_timer_freq > 500000000) {
    apic_timer_freq = 100000000; /* Fallback: 100MHz */
  }
}

void brights_apic_timer_init(uint32_t frequency_hz)
{
  if (!apic_ready) return;
  if (frequency_hz == 0) frequency_hz = 100;
  if (apic_timer_freq == 0) brights_apic_timer_calibrate();

  /* Calculate initial count */
  uint32_t init_count = apic_timer_freq / frequency_hz;
  if (init_count == 0) init_count = 1;

  /* Configure: divide by 1, periodic, vector 0x20 */
  brights_apic_write(APIC_REG_TIMER_DIV, APIC_TIMER_DIV_1);
  brights_apic_write(APIC_REG_LVT_TIMER, APIC_TIMER_PERIODIC | APIC_TIMER_VECTOR);
  brights_apic_write(APIC_REG_TIMER_INIT, init_count);
}

uint32_t brights_apic_timer_freq(void)
{
  return apic_timer_freq;
}

void brights_apic_send_ipi(uint32_t apic_id, uint8_t vector)
{
  if (!apic_ready) return;
  /* ICR high: destination APIC ID */
  brights_apic_write(APIC_REG_ICR_HI, apic_id << 24);
  /* ICR low: vector, delivery mode=fixed (0), level=0, edge trigger */
  brights_apic_write(APIC_REG_ICR_LO, vector);
}
