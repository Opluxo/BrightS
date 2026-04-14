#ifndef BRIGHTS_X86_64_TSS_H
#define BRIGHTS_X86_64_TSS_H
#include <stdint.h>
typedef struct {
  uint32_t reserved0;
  uint64_t rsp0;
  uint64_t rsp1;
  uint64_t rsp2;
  uint64_t reserved1;
  uint64_t ist1;
  uint64_t ist2;
  uint64_t ist3;
  uint64_t ist4;
  uint64_t ist5;
  uint64_t ist6;
  uint64_t ist7;
  uint64_t reserved2;
  uint16_t reserved3;
  uint16_t io_map_base;
} __attribute__((packed)) brights_tss_t;
void brights_tss_init(uint64_t rsp0);
brights_tss_t *brights_tss_ptr(void);
#endif

#ifndef BRIGHTS_X86_64_TSS_H
#define BRIGHTS_X86_64_TSS_H
#include <stdint.h>
typedef struct {
  uint32_t reserved0;
  uint64_t rsp0;
  uint64_t rsp1;
  uint64_t rsp2;
  uint64_t reserved1;
  uint64_t ist1;
  uint64_t ist2;
  uint64_t ist3;
  uint64_t ist4;
  uint64_t ist5;
  uint64_t ist6;
  uint64_t ist7;
  uint64_t reserved2;
  uint16_t reserved3;
  uint16_t io_map_base;
} __attribute__((packed)) brights_tss_t;
void brights_tss_init(uint64_t rsp0);
brights_tss_t *brights_tss_ptr(void);
#endif

#include "gdt.h"
#include "tss.h"
#include "../../drivers/serial.h"

struct gdt_entry {
  uint16_t limit_low;
  uint16_t base_low;
  uint8_t base_mid;
  uint8_t access;
  uint8_t gran;
  uint8_t base_high;
} __attribute__((packed));

struct gdt_tss_entry {
  uint16_t limit_low;
  uint16_t base_low;
  uint8_t base_mid;
  uint8_t access;
  uint8_t gran;
  uint8_t base_high;
  uint32_t base_upper;
  uint32_t reserved;
} __attribute__((packed));

struct gdt_ptr {
  uint16_t limit;
  uint64_t base;
} __attribute__((packed));

static struct {
  struct gdt_entry null;
  struct gdt_entry kcode;
  struct gdt_entry kdata;
  struct gdt_entry udata;
  struct gdt_entry ucode;
  struct gdt_tss_entry tss;
} gdt;

static struct gdt_entry make_seg(uint8_t access)
{
  struct gdt_entry e;
  e.limit_low = 0x0000;
  e.base_low = 0x0000;
  e.base_mid = 0x00;
  e.access = access;
  e.gran = 0x20; // L=1, G=0
  e.base_high = 0x00;
  return e;
}

void brights_gdt_init(void)
{
  gdt.null = make_seg(0);
  gdt.kcode = make_seg(0x9A);
  gdt.kdata = make_seg(0x92);
  gdt.udata = make_seg(0xF2);
  gdt.ucode = make_seg(0xFA);

  brights_tss_t *tss = brights_tss_ptr();
  uint64_t base = (uint64_t)tss;
  uint32_t limit = (uint32_t)(sizeof(*tss) - 1);
  gdt.tss.limit_low = limit & 0xFFFF;
  gdt.tss.base_low = base & 0xFFFF;
  gdt.tss.base_mid = (base >> 16) & 0xFF;
  gdt.tss.access = 0x89;
  gdt.tss.gran = (limit >> 16) & 0x0F;
  gdt.tss.base_high = (base >> 24) & 0xFF;
  gdt.tss.base_upper = (uint32_t)(base >> 32);
  gdt.tss.reserved = 0;

  struct gdt_ptr gp;
  gp.limit = sizeof(gdt) - 1;
  gp.base = (uint64_t)&gdt;
  __asm__ __volatile__("lgdt %0" : : "m"(gp));

  __asm__ __volatile__(
    "mov %0, %%ds\n"
    "mov %0, %%es\n"
    "mov %0, %%ss\n"
    : : "r"((uint16_t)BRIGHTS_KERNEL_DS)
  );

  uint16_t tss_sel = BRIGHTS_TSS_SEL;
  __asm__ __volatile__("ltr %0" : : "r"(tss_sel));
}
#include "tss.h"

static brights_tss_t tss;

void brights_tss_init(uint64_t rsp0)
{
  tss.rsp0 = rsp0;
  tss.io_map_base = sizeof(brights_tss_t);
}

brights_tss_t *brights_tss_ptr(void)
{
  return &tss;
}
