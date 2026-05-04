#ifndef BRIGHTS_KERNEL_ARCH_I386_PAGING_H
#define BRIGHTS_KERNEL_ARCH_I386_PAGING_H

#include <stdint.h>
#include "kernel_cpu.h"

#define BRIGHTS_I386_PAGE_SIZE    4096
#define BRIGHTS_I386_PAGE_DIR_ENTRIES 1024
#define BRIGHTS_I386_PAGE_TABLE_ENTRIES 1024

#define PTE_PRESENT    0x01
#define PTE_WRITABLE  0x02
#define PTE_USER     0x04
#define PTE_WRITETHROUGH 0x08
#define PTE_NOTCACHEABLE 0x10
#define PTE_ACCESSED   0x20
#define PTE_DIRTY    0x40
#define PTE_PAT      0x80
#define PTE_GLOBAL   0x100

#define PDE_PS_4MB   (1 << 7)
#define PDE_PS_2MB   (1 << 21)

#define PDE_PRESENT    0x01
#define PDE_WRITABLE  0x02
#define PDE_USER     0x04
#define PDE_WRITETHROUGH 0x08
#define PDE_NOTCACHEABLE 0x10
#define PDE_ACCESSED   0x20
#define PDE_DIRTY    0x40
#define PDE_PS      0x80
#define PDE_GLOBAL 0x100

typedef struct {
    uint32_t entries[BRIGHTS_I386_PAGE_DIR_ENTRIES];
} __attribute__((packed)) brights_i386_page_dir_t;

typedef struct {
    uint32_t entries[BRIGHTS_I386_PAGE_TABLE_ENTRIES];
} __attribute__((packed)) brights_i386_page_table_t;

typedef struct {
    uint32_t entries[4];
} __attribute__((packed)) brights_i386_pdpt_t;

void brights_i386_paging_init(uint32_t kernel_start, uint32_t kernel_end);
void brights_i386_paging_enable(void);
void brights_i386_paging_disable(void);
int brights_i386_paging_map(uint32_t virt, uint32_t phys, uint32_t flags);
int brights_i386_paging_unmap(uint32_t virt);
uint32_t brights_i386_paging_virt_to_phys(uint32_t virt);

#endif