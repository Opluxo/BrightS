#include "rtc.h"
#include "serial.h"
#include "../arch/x86_64/io.h"
#include <stdint.h>

#define CMOS_INDEX 0x70
#define CMOS_DATA  0x71

static uint8_t cmos_read(uint8_t reg)
{
  outb(CMOS_INDEX, reg | 0x80u);
  /* Poll COM1 LSR until TX buffer empty — known-working I/O delay in QEMU TCG.
   * Each poll iteration is ~1µs; we need ~4µs for CMOS register select. */
  { volatile uint8_t s; int i; for (i = 0; i < 16; i++) { s = inb(BRIGHTS_COM1_PORT + 5); (void)s; } }
  return inb(CMOS_DATA);
}

static uint8_t bcd_to_bin(uint8_t v)
{
  return (uint8_t)((v & 0x0F) + ((v >> 4) * 10));
}

int brights_rtc_read(brights_rtc_time_t *t)
{
  if (!t) {
    return -1;
  }

  /* Read RTC registers. Skip UIP check to avoid TCG stalls. */
  uint8_t sec = cmos_read(0x00);
  uint8_t min = cmos_read(0x02);
  uint8_t hour = cmos_read(0x04);
  uint8_t day = cmos_read(0x07);
  uint8_t mon = cmos_read(0x08);
  uint8_t yr = cmos_read(0x09);
  uint8_t regb = cmos_read(0x0B);

  int is_bcd = ((regb & 0x04u) == 0);
  if (is_bcd) {
    sec = bcd_to_bin(sec);
    min = bcd_to_bin(min);
    hour = bcd_to_bin(hour & 0x7Fu);
    day = bcd_to_bin(day);
    mon = bcd_to_bin(mon);
    yr = bcd_to_bin(yr);
  } else {
    hour &= 0x7Fu;
  }

  t->second = sec;
  t->minute = min;
  t->hour = hour;
  t->day = day;
  t->month = mon;
  t->year = (uint16_t)(2000u + yr);
  return 0;
}
