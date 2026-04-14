#ifndef BRIGHTS_X86_64_CPU_LOCAL_H
#define BRIGHTS_X86_64_CPU_LOCAL_H

#include <stdint.h>

/* Initialize per-CPU data for BSP */
void brights_cpu_local_init(uint64_t kernel_rsp);

/* Initialize per-CPU data for an AP */
void brights_cpu_local_init_ap(uint32_t apic_id, uint64_t kernel_rsp);

/* Get current CPU's APIC ID */
uint32_t brights_cpu_local_id(void);

/* Get current process PID on this CPU */
uint32_t brights_cpu_local_current_pid(void);

/* Set current process PID on this CPU */
void brights_cpu_local_set_pid(uint32_t pid);

#endif
#include "cpu_local.h"
#include "msr.h"

#define IA32_KERNEL_GS_BASE 0xC0000102u
#define IA32_GS_BASE        0xC0000101u

/*
 * Per-CPU data structure, accessed via GS base.
 *
 * Layout (64-byte aligned for cache line):
 *   [0x00] kernel_rsp    (used by syscall_entry.S for SWAPGS)
 *   [0x08] cpu_id        (APIC ID)
 *   [0x10] current_pid   (current process on this CPU)
 *   [0x18] tick_count    (per-CPU tick counter)
 *   [0x20] preempt_count (preemption disable depth)
 *   [0x28] reserved
 */
typedef struct {
  uint64_t kernel_rsp;    /* Kernel stack pointer for syscall entry */
  uint32_t cpu_id;        /* APIC ID of this CPU */
  uint32_t current_pid;   /* Currently running process */
  uint64_t tick_count;    /* Per-CPU tick counter */
  uint32_t preempt_count; /* Preemption disable depth */
  uint32_t reserved;
} __attribute__((aligned(64))) brights_cpu_local_t;

static brights_cpu_local_t cpu_data[8]; /* Up to 8 CPUs */

void brights_cpu_local_init(uint64_t kernel_rsp)
{
  cpu_data[0].kernel_rsp = kernel_rsp;
  cpu_data[0].cpu_id = 0;
  cpu_data[0].current_pid = 0;
  cpu_data[0].tick_count = 0;
  cpu_data[0].preempt_count = 0;

  /* Set GS base to point to this CPU's data */
  wrmsr(IA32_KERNEL_GS_BASE, (uint64_t)(uintptr_t)&cpu_data[0]);
  wrmsr(IA32_GS_BASE, (uint64_t)(uintptr_t)&cpu_data[0]);
}

void brights_cpu_local_init_ap(uint32_t apic_id, uint64_t kernel_rsp)
{
  if (apic_id >= 8) return;
  cpu_data[apic_id].kernel_rsp = kernel_rsp;
  cpu_data[apic_id].cpu_id = apic_id;
  cpu_data[apic_id].current_pid = 0;
  cpu_data[apic_id].tick_count = 0;
  cpu_data[apic_id].preempt_count = 0;

  wrmsr(IA32_KERNEL_GS_BASE, (uint64_t)(uintptr_t)&cpu_data[apic_id]);
  wrmsr(IA32_GS_BASE, (uint64_t)(uintptr_t)&cpu_data[apic_id]);
}

uint32_t brights_cpu_local_id(void)
{
  uint32_t eax, ebx, ecx, edx;
  __asm__ __volatile__("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
  return (ebx >> 24) & 0xFF;
}

uint32_t brights_cpu_local_current_pid(void)
{
  return cpu_data[0].current_pid;
}

void brights_cpu_local_set_pid(uint32_t pid)
{
  cpu_data[0].current_pid = pid;
}
