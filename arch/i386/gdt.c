#include "gdt.h"
#include "../../kernel/core/sched.h"
#include "../../drivers/serial.h"

struct gdt_entry
{
  uint16_t limit_low;
  uint16_t base_low;
  uint8_t base_mid;
  uint8_t access;
  uint8_t gran;
  uint8_t base_high;
} __attribute__((packed));

struct gdt_tss_entry
{
  uint16_t limit_low;
  uint16_t base_low;
  uint8_t base_mid;
  uint8_t access;
  uint8_t gran;
  uint8_t base_high;
} __attribute__((packed));

struct gdt_ptr
{
  uint16_t limit;
  uint32_t base;
} __attribute__((packed));

static struct
{
  struct gdt_entry null;
  struct gdt_entry kcode;
  struct gdt_entry kdata;
  struct gdt_entry udata;
  struct gdt_entry ucode;
  struct gdt_tss_entry tss;
} gdt;

/* i386 TSS (104 bytes) */
typedef struct
{
  uint16_t prev_link;
  uint16_t reserved0;
  uint32_t esp0;
  uint16_t ss0;
  uint16_t reserved1;
  uint32_t esp1;
  uint16_t ss1;
  uint16_t reserved2;
  uint32_t esp2;
  uint16_t ss2;
  uint16_t reserved3;
  uint32_t cr3;
  uint32_t eip;
  uint32_t eflags;
  uint32_t eax, ecx, edx, ebx;
  uint32_t esp, ebp, esi, edi;
  uint16_t es, reserved4;
  uint16_t cs, reserved5;
  uint16_t ss, reserved6;
  uint16_t ds, reserved7;
  uint16_t fs, reserved8;
  uint16_t gs, reserved9;
  uint16_t ldt, reserved10;
  uint16_t trap;
  uint16_t io_map_base;
} __attribute__((packed)) brights_tss_t;

static brights_tss_t tss;

void brights_tss_init(uint32_t esp0)
{
  tss.ss0 = BRIGHTS_KERNEL_DS;
  tss.esp0 = esp0;
  tss.io_map_base = sizeof(brights_tss_t);
}

uint32_t brights_tss_esp0(void)
{
  return tss.esp0;
}

static void make_seg(struct gdt_entry *e, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
  e->limit_low = limit & 0xFFFF;
  e->base_low = base & 0xFFFF;
  e->base_mid = (base >> 16) & 0xFF;
  e->access = access;
  e->gran = (limit >> 16) & 0x0F;
  e->gran |= gran & 0xF0;
  e->base_high = (base >> 24) & 0xFF;
}

static void make_tss_seg(struct gdt_tss_entry *e, uint32_t base, uint32_t limit)
{
  e->limit_low = limit & 0xFFFF;
  e->base_low = base & 0xFFFF;
  e->base_mid = (base >> 16) & 0xFF;
  e->access = 0x89;
  e->gran = (limit >> 16) & 0x0F;
  e->base_high = (base >> 24) & 0xFF;
}

void brights_gdt_init(void)
{
  make_seg(&gdt.null, 0, 0, 0, 0);
  make_seg(&gdt.kcode, 0, 0xFFFFF, 0x9A, 0xCF);
  make_seg(&gdt.kdata, 0, 0xFFFFF, 0x92, 0xCF);
  make_seg(&gdt.udata, 0, 0xFFFFF, 0xF2, 0xCF);
  make_seg(&gdt.ucode, 0, 0xFFFFF, 0xFA, 0xCF);

  uint32_t tss_base = (uint32_t)(uintptr_t)&tss;
  uint32_t tss_limit = sizeof(brights_tss_t) - 1;
  make_tss_seg(&gdt.tss, tss_base, tss_limit);

  struct gdt_ptr gp;
  gp.limit = sizeof(gdt) - 1;
  gp.base = (uint32_t)&gdt;
  __asm__ __volatile__("lgdt %0" : : "m"(gp));

  __asm__ __volatile__(
    "mov %0, %%ds\n"
    "mov %0, %%es\n"
    "mov %0, %%ss\n"
    : : "r"((uint16_t)BRIGHTS_KERNEL_DS)
  );

  uint16_t tss_sel = BRIGHTS_I386_TSS_SEL;
  __asm__ __volatile__("ltr %0" : : "r"(tss_sel));
}
