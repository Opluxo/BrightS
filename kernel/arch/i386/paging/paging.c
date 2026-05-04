#include "../include/paging.h"

#define MAX_PAGE_TABLES 32

static brights_i386_pdpt_t pdpt __attribute__((aligned(32)));
static brights_i386_page_dir_t page_dir __attribute__((aligned(4096)));
static brights_i386_page_table_t page_tables[MAX_PAGE_TABLES] __attribute__((aligned(4096)));

void brights_i386_paging_init(uint32_t kernel_start, uint32_t kernel_end)
{
    // Initialize page structures to zero
    for (int i = 0; i < 4; i++) {
        pdpt.entries[i] = 0;
    }
    
    for (int i = 0; i < BRIGHTS_I386_PAGE_DIR_ENTRIES; i++) {
        page_dir.entries[i] = 0;
    }
    
    for (int t = 0; t < MAX_PAGE_TABLES; t++) {
        for (int i = 0; i < BRIGHTS_I386_PAGE_TABLE_ENTRIES; i++) {
            page_tables[t].entries[i] = 0;
        }
    }
    
    // Calculate number of page tables needed
    uint32_t kernel_size = kernel_end - kernel_start;
    uint32_t num_page_tables = ((kernel_size + 0x1000000) + 0x3FFFFF) / 0x400000;
    if (num_page_tables > MAX_PAGE_TABLES) {
        num_page_tables = MAX_PAGE_TABLES;
    }
    
    // Map kernel pages
    for (uint32_t i = 0; i < num_page_tables; i++) {
        uint32_t phys_addr = 0x100000 + i * 0x400000;
        
        page_dir.entries[i] = (uint32_t)&page_tables[i] | PDE_PRESENT | PDE_WRITABLE | PDE_ACCESSED;
        
        // Map 4MB of memory starting from 1MB
        for (uint32_t j = 0; j < BRIGHTS_I386_PAGE_TABLE_ENTRIES && phys_addr < 0x200000; j++) {
            uint32_t page_phys = phys_addr + j * 4096;
            page_tables[i].entries[j] = page_phys | PTE_PRESENT | PTE_WRITABLE | PTE_ACCESSED;
        }
    }
    
    // Set up PDPT
    pdpt.entries[0] = (uint32_t)&page_dir | 1;
    
    // Enable PAE and paging
    uint32_t cr4 = brights_i386_read_cr4();
    cr4 |= CR4_PAE;
    brights_i386_write_cr4(cr4);
    
    brights_i386_write_cr3((uint32_t)&pdpt);
    
    uint32_t cr0 = brights_i386_read_cr0();
    cr0 |= CR0_PG;
    brights_i386_write_cr0(cr0);
}

void brights_i386_paging_enable(void)
{
    uint32_t cr0 = brights_i386_read_cr0();
    cr0 |= CR0_PG;
    brights_i386_write_cr0(cr0);
}

void brights_i386_paging_disable(void)
{
    uint32_t cr0 = brights_i386_read_cr0();
    cr0 &= ~CR0_PG;
    brights_i386_write_cr0(cr0);
}

int brights_i386_paging_map(uint32_t virt, uint32_t phys, uint32_t flags)
{
    uint32_t pde_idx = (virt >> 22) & 0x3FF;
    uint32_t pte_idx = (virt >> 12) & 0x3FF;
    
    if (pde_idx >= BRIGHTS_I386_PAGE_DIR_ENTRIES || pte_idx >= BRIGHTS_I386_PAGE_TABLE_ENTRIES) {
        return -1;
    }
    
    // Allocate page table if needed
    if (page_dir.entries[pde_idx] == 0) {
        for (int t = 0; t < MAX_PAGE_TABLES; t++) {
            int unused = 1;
            for (int i = 0; i < BRIGHTS_I386_PAGE_TABLE_ENTRIES; i++) {
                if (page_tables[t].entries[i] & PTE_PRESENT) {
                    unused = 0;
                    break;
                }
            }
            if (unused) {
                page_dir.entries[pde_idx] = (uint32_t)&page_tables[t] | PDE_PRESENT | PDE_WRITABLE;
                break;
            }
        }
    }
    
    if (page_dir.entries[pde_idx] == 0) {
        return -1;
    }
    
    uint32_t *pt = (uint32_t*)(page_dir.entries[pde_idx] & ~0xFFF);
    pt[pte_idx] = phys | flags | PTE_PRESENT | PTE_ACCESSED;
    
    brights_i386_invlpg((void*)virt);
    
    return 0;
}

int brights_i386_paging_unmap(uint32_t virt)
{
    uint32_t pde_idx = (virt >> 22) & 0x3FF;
    uint32_t pte_idx = (virt >> 12) & 0x3FF;
    
    if (pde_idx >= BRIGHTS_I386_PAGE_DIR_ENTRIES || pte_idx >= BRIGHTS_I386_PAGE_TABLE_ENTRIES) {
        return -1;
    }
    
    if (page_dir.entries[pde_idx] == 0) {
        return -1;
    }
    
    uint32_t *pt = (uint32_t*)(page_dir.entries[pde_idx] & ~0xFFF);
    pt[pte_idx] = 0;
    
    brights_i386_invlpg((void*)virt);
    
    return 0;
}

uint32_t brights_i386_paging_virt_to_phys(uint32_t virt)
{
    uint32_t pde_idx = (virt >> 22) & 0x3FF;
    uint32_t pte_idx = (virt >> 12) & 0x3FF;
    
    if (pde_idx >= BRIGHTS_I386_PAGE_DIR_ENTRIES) {
        return 0;
    }
    
    if (page_dir.entries[pde_idx] == 0) {
        return 0;
    }
    
    uint32_t *pt = (uint32_t*)(page_dir.entries[pde_idx] & ~0xFFF);
    if ((pt[pte_idx] & PTE_PRESENT) == 0) {
        return 0;
    }
    
    return (pt[pte_idx] & ~0xFFF) | (virt & 0xFFF);
}