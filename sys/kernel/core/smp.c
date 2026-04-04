#include "smp.h"
#include "hwinfo.h"
#include "printf.h"
#include "../platform/x86_64/apic.h"
#include "../platform/x86_64/msr.h"
#include "../platform/x86_64/io.h"
#include "../dev/serial.h"
#include <stdint.h>

/*
 * SMP Bootstrap for i5-1135G7 (4 cores / 8 threads).
 *
 * AP trampoline is loaded at physical address 0x8000.
 * Each AP:
 *   1. Receives INIT IPI → starts in real mode at 0x0000:0x8000
 *   2. Trampoline sets up GDT, enables paging, enters long mode
 *   3. Jumps to brights_smp_ap_entry()
 */

#define AP_TRAMPOLINE_ADDR  0x8000
#define AP_STACK_SIZE       4096
#define MAX_CPUS            8

static uint32_t smp_core_count = 1;
static uint32_t smp_apic_ids[MAX_CPUS];
static volatile uint32_t ap_startup_count = 1; /* BSP already running */

/* Per-AP stacks */
static uint8_t ap_stacks[MAX_CPUS][AP_STACK_SIZE] __attribute__((aligned(16)));

/* The AP trampoline code (16-bit real mode → 64-bit long mode).
 * This is hand-assembled machine code loaded at AP_TRAMPOLINE_ADDR.
 *
 * Layout:
 *   0x8000: Real mode entry
 *   0x8010: GDT pointer
 *   0x8020: GDT entries
 *   0x8040: PML4 address (patched at runtime)
 *   0x8048: AP entry address (patched at runtime)
 *   0x8050: Stack top per APIC ID (patched at runtime)
 *   0x8080: 32-bit protected mode code
 *   0x8100: 64-bit code
 */
static const uint8_t ap_trampoline[] = {
  /* ===== 0x8000: Real mode entry ===== */
  0xFA,                         /* cli */
  0x31, 0xC0,                   /* xor ax, ax */
  0x8E, 0xD8,                   /* mov ds, ax */
  0x8E, 0xC0,                   /* mov es, ax */
  0x8E, 0xD0,                   /* mov ss, ax */

  /* Load GDT (at offset 0x8010) */
  0x0F, 0x01, 0x16, 0x10, 0x80,  /* lgdt [0x8010] */

  /* Enable protected mode: set CR0.PE */
  0x0F, 0x20, 0xC0,             /* mov eax, cr0 */
  0x66, 0x83, 0xC8, 0x01,       /* or eax, 1 */
  0x0F, 0x22, 0xC0,             /* mov cr0, eax */

  /* Far jump to 32-bit code */
  0x66, 0xEA,
  0x80, 0x81, 0x00, 0x00,       /* offset: 0x8180 */
  0x08, 0x00,                   /* selector: 0x08 (code32) */

  /* ===== 0x8010: GDT descriptor ===== */
  0x2F, 0x00,                   /* limit: 47 (6 entries * 8 - 1) */
  0x20, 0x80, 0x00, 0x00,       /* base: 0x8020 */

  /* ===== 0x8016: padding to 0x8020 ===== */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

  /* ===== 0x8020: GDT ===== */
  /* 0x00: null */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* 0x08: code32 (exec/read, 32-bit) */
  0xFF, 0xFF, 0x00, 0x00, 0x00, 0x9A, 0xCF, 0x00,
  /* 0x10: data32 (read/write) */
  0xFF, 0xFF, 0x00, 0x00, 0x00, 0x92, 0xCF, 0x00,
  /* 0x18: code64 (exec/read, L-bit) */
  0xFF, 0xFF, 0x00, 0x00, 0x00, 0x9A, 0xAF, 0x00,
  /* 0x20: null (placeholder for TSS) */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

  /* ===== 0x8040: PML4 address (patched by BSP) ===== */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

  /* ===== 0x8048: AP entry point (patched by BSP) ===== */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

  /* ===== 0x8050: AP stack top (patched by BSP) ===== */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

  /* ===== 0x8058-0x807F: padding ===== */
  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,

  /* ===== 0x8080: 32-bit protected mode entry ===== */
  /* Set up segments */
  0x66, 0xB8, 0x10, 0x00,       /* mov ax, 0x10 (data seg) */
  0x8E, 0xD8,                   /* mov ds, ax */
  0x8E, 0xC0,                   /* mov es, ax */
  0x8E, 0xD0,                   /* mov ss, ax */

  /* Read APIC ID to determine which AP this is */
  0xB8, 0x00, 0x00, 0x00, 0x00, /* mov eax, 0 (placeholder for APIC base low) */
  0x0F, 0x20, 0xD8,             /* mov ebx, cr3 - we'll read APIC ID instead */

  /* Load CR3 with PML4 from 0x8040 */
  0xA1, 0x40, 0x80, 0x00, 0x00, /* mov eax, [0x8040] */
  0x0F, 0x22, 0xD8,             /* mov cr3, eax */

  /* Enable PAE */
  0x0F, 0x20, 0xE0,             /* mov eax, cr4 */
  0x0D, 0x00, 0x00, 0x20, 0x00, /* or eax, 0x200000 (PAE) */
  0x0F, 0x22, 0xE0,             /* mov cr4, eax */

  /* Enable long mode via EFER MSR */
  0xB9, 0x80, 0x00, 0x00, 0xC0, /* mov ecx, 0xC0000080 (EFER) */
  0x0F, 0x32,                   /* rdmsr */
  0x0D, 0x00, 0x01, 0x00, 0x00, /* or eax, 0x100 (LME) */
  0x0F, 0x30,                   /* wrmsr */

  /* Enable paging */
  0x0F, 0x20, 0xC0,             /* mov eax, cr0 */
  0x0D, 0x00, 0x00, 0x80, 0x00, /* or eax, 0x80000000 (PG) */
  0x0F, 0x22, 0xC0,             /* mov cr0, eax */

  /* Far jump to 64-bit code */
  0xEA,
  0xA0, 0x81, 0x00, 0x00,       /* offset: 0x81A0 */
  0x18, 0x00,                   /* selector: 0x18 (code64) */

  /* ===== 0x80C0-0x80FF: padding ===== */
  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,

  /* ===== 0x8100: 64-bit mode code ===== */
  /* Read APIC ID from APIC MMIO + 0x20 (shifted right 24) */
  0x48, 0x8B, 0x05, 0x31, 0xFF, 0xFF, 0xFF,  /* mov rax, [rip-207] -> 0x8040 (PML4, but actually we need APIC base) */
  /* Actually, let's use CPUID to get APIC ID (simpler) */
  0xB8, 0x01, 0x00, 0x00, 0x00, /* mov eax, 1 */
  0x0F, 0xA2,                   /* cpuid */
  0xC1, 0xEB, 0x18,             /* shr ebx, 24 */
  /* ebx now has APIC ID */

  /* Load stack based on APIC ID */
  /* We'll use a simple approach: AP0=stack[0], AP1=stack[1], etc. */
  0x48, 0x8D, 0x05, 0x28, 0xFF, 0xFF, 0xFF,  /* lea rax, [rip-216] -> 0x8050 (stack base) */
  0x48, 0x8B, 0x00,             /* mov rax, [rax] */
  0x48, 0x89, 0xC4,             /* mov rsp, rax */

  /* Call the AP C entry with APIC ID as argument */
  0x48, 0x89, 0xDF,             /* mov rdi, rbx (APIC ID) */
  0x48, 0x8B, 0x05, 0x0D, 0xFF, 0xFF, 0xFF,  /* mov rax, [rip-243] -> 0x8048 (entry point) */
  0xFF, 0xD0,                   /* call rax */

  /* Halt if we return */
  0xFA,                         /* cli */
  0xF4,                         /* hlt */
  0xEB, 0xFD,                   /* jmp $ */
};

static void smp_debug(const char *msg)
{
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, msg);
}

int brights_smp_init(void)
{
  if (!brights_apic_available()) {
    smp_debug("smp: no APIC, single core\r\n");
    return 0;
  }

  const brights_cpu_info_t *cpu = brights_hwinfo_cpu();
  uint32_t cores = cpu->cores_per_pkg;
  if (cores > MAX_CPUS) cores = MAX_CPUS;
  if (cores <= 1) {
    smp_debug("smp: single core detected\r\n");
    return 0;
  }

  smp_debug("smp: attempting to start ");
  /* Print core count - inline to avoid deps */
  char c = '0' + cores;
  char cs[2] = {c, 0};
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, cs);
  smp_debug(" cores\r\n");

  /* Copy trampoline to low memory */
  uint8_t *tramp = (uint8_t *)(uintptr_t)AP_TRAMPOLINE_ADDR;
  for (unsigned i = 0; i < sizeof(ap_trampoline); ++i) {
    tramp[i] = ap_trampoline[i];
  }

  /* Get current CR3 (page table) */
  uint64_t cr3;
  __asm__ __volatile__("mov %%cr3, %0" : "=r"(cr3));

  /* Patch PML4 address at offset 0x8040 */
  *(uint64_t *)(tramp + 0x40) = cr3 & ~0xFFFULL;

  /* Patch AP entry point at offset 0x8048 */
  *(uint64_t *)(tramp + 0x48) = (uint64_t)(uintptr_t)brights_smp_ap_entry;

  /* Patch stack top at offset 0x8050 */
  *(uint64_t *)(tramp + 0x50) = (uint64_t)(uintptr_t)&ap_stacks[1][AP_STACK_SIZE];

  /* Record BSP APIC ID */
  smp_apic_ids[0] = brights_apic_id();

  /* Send INIT IPI to all APs */
  brights_apic_write(0x310, 0); /* ICR high: destination = all */
  brights_apic_write(0x300, 0x000C4500); /* INIT, all excluding self, level=1, assert */

  /* Wait 10ms */
  for (volatile int i = 0; i < 1000000; ++i) __asm__ __volatile__("pause");

  /* De-assert INIT */
  brights_apic_write(0x310, 0);
  brights_apic_write(0x300, 0x000C4500 & ~(1u << 14)); /* level=0 */

  /* Wait another 10ms */
  for (volatile int i = 0; i < 1000000; ++i) __asm__ __volatile__("pause");

  /* Send SIPI (Start-up IPI) to each AP */
  for (uint32_t i = 1; i < cores; ++i) {
    /* Patch stack for this AP */
    *(uint64_t *)(tramp + 0x50) = (uint64_t)(uintptr_t)&ap_stacks[i][AP_STACK_SIZE];

    /* SIPI: vector = AP_TRAMPOLINE_ADDR / 4096 = 0x08 */
    brights_apic_write(0x310, 0); /* broadcast */
    brights_apic_write(0x300, 0x000C4608); /* SIPI, vector 0x08, level=1 */

    /* Wait 200us */
    for (volatile int j = 0; j < 20000; ++j) __asm__ __volatile__("pause");

    /* Second SIPI if needed */
    brights_apic_write(0x310, 0);
    brights_apic_write(0x300, 0x000C4608);
    for (volatile int j = 0; j < 200000; ++j) __asm__ __volatile__("pause");
  }

  /* Wait for APs to start (timeout 1 second) */
  for (volatile int i = 0; i < 100000000; ++i) {
    if (ap_startup_count >= cores) break;
    __asm__ __volatile__("pause");
  }

  smp_core_count = ap_startup_count;
  smp_debug("smp: ");
  char n = '0' + smp_core_count;
  char ns[2] = {n, 0};
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, ns);
  smp_debug(" cores active\r\n");

  return smp_core_count;
}

/* Called by each AP when it boots */
void brights_smp_ap_entry(uint32_t apic_id)
{
  uint32_t idx = __sync_fetch_and_add(&ap_startup_count, 1);
  smp_apic_ids[idx] = apic_id;

  /* Enable APIC on this core */
  brights_apic_eoi();
  brights_apic_write(0x80, 0); /* TPR = 0 */

  /* Halt - wait for IPI from scheduler */
  for (;;) {
    __asm__ __volatile__("cli; hlt");
  }
}

uint32_t brights_smp_core_count(void)
{
  return smp_core_count;
}

uint32_t brights_smp_apic_id(uint32_t core_idx)
{
  if (core_idx >= smp_core_count) return 0;
  return smp_apic_ids[core_idx];
}
