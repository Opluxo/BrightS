#include "pf.h"
#include "../drivers/serial.h"
#include "proc.h"
#include "printf.h"

int brights_page_fault_handler(uint64_t fault_addr, uint64_t error_code)
{
  uint32_t pid = brights_proc_current();

  /* Check if it's a valid user-space address */
  if (fault_addr < 0x0000800000000000ULL) {
    /* User-space page fault - could be demand paging */
    /* For now, return -1 to indicate unhandled */
    return -1;
  }

  /* Kernel-space or invalid page fault */
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "pf: pid=");
  char buf[24];
  int len = 0;
  uint64_t v = pid;
  if (v == 0) { buf[0] = '0'; len = 1; }
  else { while (v > 0) { buf[len++] = '0' + (v % 10); v /= 10; } }
  for (int i = 0; i < len / 2; ++i) { char t = buf[i]; buf[i] = buf[len-1-i]; buf[len-1-i] = t; }
  buf[len] = 0;
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, buf);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, " addr=0x");

  static const char hex[] = "0123456789ABCDEF";
  char hexbuf[20];
  hexbuf[0] = '0'; hexbuf[1] = 'x';
  for (int i = 0; i < 16; ++i) hexbuf[2 + i] = hex[(fault_addr >> ((15 - i) * 4)) & 0xF];
  hexbuf[18] = 0;
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, hexbuf);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, " err=0x");
  hexbuf[2] = hex[(error_code >> 4) & 0xF];
  hexbuf[3] = hex[error_code & 0xF];
  hexbuf[4] = 0;
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, hexbuf);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\r\n");

  return -1;
}
