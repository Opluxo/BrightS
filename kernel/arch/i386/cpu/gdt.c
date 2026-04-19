#include "../include/kernel_cpu.h"

#define GDT_ENTRIES 8
#define GDT_KERNEL_CODE 1
#define GDT_KERNEL_DATA 2
#define GDT_USER_CODE   3
#define GDT_USER_DATA   4
#define GDT_TSS        5

typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) gdt_ptr_t;

static gdt_entry_t gdt[GDT_ENTRIES];
static gdt_ptr_t gdt_ptr;
static uint8_t tss[104];

extern uint32_t brights_i386_kernel_esp;

void brights_i386_gdt_init(void)
{
    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base = (uint32_t)gdt;
    
    gdt[0].limit_low = 0;
    gdt[0].base_low = 0;
    gdt[0].base_middle = 0;
    gdt[0].access = 0;
    gdt[0].granularity = 0;
    gdt[0].base_high = 0;
    
    gdt[GDT_KERNEL_CODE].limit_low = 0xFFFF;
    gdt[GDT_KERNEL_CODE].base_low = 0x0000;
    gdt[GDT_KERNEL_CODE].base_middle = 0x00;
    gdt[GDT_KERNEL_CODE].access = 0x9A;
    gdt[GDT_KERNEL_CODE].granularity = 0xCF;
    gdt[GDT_KERNEL_CODE].base_high = 0x00;
    
    gdt[GDT_KERNEL_DATA].limit_low = 0xFFFF;
    gdt[GDT_KERNEL_DATA].base_low = 0x0000;
    gdt[GDT_KERNEL_DATA].base_middle = 0x00;
    gdt[GDT_KERNEL_DATA].access = 0x92;
    gdt[GDT_KERNEL_DATA].granularity = 0xCF;
    gdt[GDT_KERNEL_DATA].base_high = 0x00;
    
    gdt[GDT_USER_CODE].limit_low = 0xFFFF;
    gdt[GDT_USER_CODE].base_low = 0x0000;
    gdt[GDT_USER_CODE].base_middle = 0x00;
    gdt[GDT_USER_CODE].access = 0xFA;
    gdt[GDT_USER_CODE].granularity = 0xCF;
    gdt[GDT_USER_CODE].base_high = 0x00;
    
    gdt[GDT_USER_DATA].limit_low = 0xFFFF;
    gdt[GDT_USER_DATA].base_low = 0x0000;
    gdt[GDT_USER_DATA].base_middle = 0x00;
    gdt[GDT_USER_DATA].access = 0xF2;
    gdt[GDT_USER_DATA].granularity = 0xCF;
    gdt[GDT_USER_DATA].base_high = 0x00;
    
    *(uint32_t*)(tss + 4) = (uint32_t)&brights_i386_kernel_esp;
    *(uint16_t*)(tss + 8) = 0x10;
    *(uint16_t*)(tss + 10) = 0x89;
    *(uint16_t*)(tss + 12) = 0x00;
    *(uint32_t*)(tss + 36) = (uint32_t)tss + 104;
    
    gdt[GDT_TSS].limit_low = sizeof(tss) - 1;
    gdt[GDT_TSS].base_low = (uint32_t)tss & 0xFFFF;
    gdt[GDT_TSS].base_middle = ((uint32_t)tss >> 16) & 0xFF;
    gdt[GDT_TSS].access = 0xE9;
    gdt[GDT_TSS].granularity = 0x00;
    gdt[GDT_TSS].base_high = ((uint32_t)tss >> 24) & 0xFF;
    
    brights_i386_lgdt(&gdt_ptr, sizeof(gdt_ptr));
    
    uint32_t cr0 = brights_i386_read_cr0();
    cr0 |= CR0_PE;
    brights_i386_write_cr0(cr0);
    
    __asm__ __volatile__(
        "ljmp $0x08, $1f\n"
        "1:\n"
        "mov $0x10, %ax\n"
        "mov %ax, %ds\n"
        "mov %ax, %es\n"
        "mov %ax, %fs\n"
        "mov %ax, %gs\n"
        "mov %ax, %ss\n"
        "mov %ax, %cs\n"
    );
}