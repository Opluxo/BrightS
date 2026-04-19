#ifndef BRIGHTS_KERNEL_ARCH_I386_CPU_H
#define BRIGHTS_KERNEL_ARCH_I386_CPU_H

#include <stdint.h>

#define CR0_PG (1 << 31)
#define CR0_WP (1 << 16)
#define CR0_NE (1 << 5)
#define CR0_ET (1 << 4)
#define CR0_TS (1 << 3)
#define CR0_EM (1 << 2)
#define CR0_MP (1 << 1)
#define CR0_PE (1 << 0)

#define CR4_PSE (1 << 4)
#define CR4_PGE (1 << 7)
#define CR4_PAE (1 << 5)
#define CR4_DE  (1 << 3)
#define CR4_TSD (1 << 2)
#define CR4_PVI (1 << 1)
#define CR4_VME (1 << 0)

#define EFLAGS_CF (1 << 0)
#define EFLAGS_PF (1 << 2)
#define EFLAGS_AF (1 << 4)
#define EFLAGS_ZF (1 << 6)
#define EFLAGS_SF (1 << 7)
#define EFLAGS_TF (1 << 8)
#define EFLAGS_IF (1 << 9)
#define EFLAGS_DF (1 << 10)
#define EFLAGS_OF (1 << 11)
#define EFLAGS_IOPL (3 << 12)
#define EFLAGS_NT (1 << 14)
#define EFLAGS_RF (1 << 16)
#define EFLAGS_VM (1 << 17)
#define EFLAGS_AC (1 << 18)
#define EFLAGS_VIF (1 << 19)
#define EFLAGS_ID (1 << 21)

#define CPU_HAS_PAE    (1 << 6)
#define CPU_HAS_PSE36  (1 << 17)

#define R_EAX 0
#define R_ECX 1
#define R_EDX 2
#define R_EBX 3
#define R_ESP 4
#define R_EBP 5
#define R_ESI 6
#define R_EDI 7

typedef struct {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t eip, cs, eflags, esp32, ss32;
} __attribute__((packed)) brights_i386_trap_frame_t;

typedef struct {
    uint32_t cr0, cr2, cr3, cr4;
    uint32_t cs, ds, es, fs, gs, ss;
    uint32_t eflags;
    uint32_t eip;
    uint32_t esp;
    uint32_t cr0_orig;
} brights_i386_cpu_state_t;

void brights_i386_gdt_init(void);
void brights_i386_idt_init(void);
void brights_i386_paging_init(uint32_t kernel_cr3);
void brights_i386_enable_paging(void);
uint32_t brights_i386_get_cr3(void);
void brights_i386_set_cr3(uint32_t cr3);

static inline uint32_t brights_i386_read_cr0(void)
{
    uint32_t val;
    __asm__ __volatile__("mov %%cr0, %0" : "=r"(val));
    return val;
}

static inline void brights_i386_write_cr0(uint32_t val)
{
    __asm__ __volatile__("mov %0, %%cr0" : : "r"(val));
}

static inline uint32_t brights_i386_read_cr2(void)
{
    uint32_t val;
    __asm__ __volatile__("mov %%cr2, %0" : "=r"(val));
    return val;
}

static inline uint32_t brights_i386_read_cr3(void)
{
    uint32_t val;
    __asm__ __volatile__("mov %%cr3, %0" : "=r"(val));
    return val;
}

static inline void brights_i386_write_cr3(uint32_t val)
{
    __asm__ __volatile__("mov %0, %%cr3" : : "r"(val));
}

static inline uint32_t brights_i386_read_cr4(void)
{
    uint32_t val;
    __asm__ __volatile__("mov %%cr4, %0" : "=r"(val));
    return val;
}

static inline void brights_i386_write_cr4(uint32_t val)
{
    __asm__ __volatile__("mov %0, %%cr4" : : "r"(val));
}

static inline void brights_i386_cli(void)
{
    __asm__ __volatile__("cli");
}

static inline void brights_i386_sti(void)
{
    __asm__ __volatile__("sti");
}

static inline uint32_t brights_i386_read_eflags(void)
{
    uint32_t val;
    __asm__ __volatile__("pushf; pop %0" : "=r"(val));
    return val;
}

static inline void brights_i386_write_eflags(uint32_t val)
{
    __asm__ __volatile__("push %0; popf" : : "r"(val));
}

static inline void brights_i386_invlpg(void *addr)
{
    __asm__ __volatile__("invlpg %0" : : "m"(addr));
}

static inline void brights_i386_hlt(void)
{
    __asm__ __volatile__("hlt");
}

static inline void brights_i386_lgdt(void *ptr, uint16_t limit)
{
    __asm__ __volatile__("lgdt %0" : : "m"(*(uint16_t*)&limit), "m"(*(uint32_t*)ptr));
}

static inline void brights_i386_lidt(void *ptr, uint16_t limit)
{
    __asm__ __volatile__("lidt %0" : : "m"(*(uint16_t*)&limit), "m"(*(uint32_t*)ptr));
}

#endif