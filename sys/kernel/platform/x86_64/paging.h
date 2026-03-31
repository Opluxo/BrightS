#ifndef BRIGHTS_X86_64_PAGING_H
#define BRIGHTS_X86_64_PAGING_H

#include <stdint.h>

// Page size constants
#define BRIGHTS_PAGE_SIZE_4K 4096ULL
#define BRIGHTS_PAGE_SIZE_2M (2ULL * 1024 * 1024)
#define BRIGHTS_PAGE_SIZE_1G (1ULL * 1024 * 1024 * 1024)

// Page table entry flags
#define BRIGHTS_PTE_PRESENT   (1ULL << 0)
#define BRIGHTS_PTE_WRITABLE  (1ULL << 1)
#define BRIGHTS_PTE_USER      (1ULL << 2)
#define BRIGHTS_PTE_WT        (1ULL << 3)
#define BRIGHTS_PTE_CD        (1ULL << 4)
#define BRIGHTS_PTE_ACCESSED  (1ULL << 5)
#define BRIGHTS_PTE_DIRTY     (1ULL << 6)
#define BRIGHTS_PTE_HUGE      (1ULL << 7)
#define BRIGHTS_PTE_GLOBAL    (1ULL << 8)
#define BRIGHTS_PTE_NX        (1ULL << 63)

// Initialize paging subsystem
void brights_paging_init(void);

// Map a virtual page to a physical page
int brights_paging_map(uint64_t virt, uint64_t phys, uint64_t flags);

// Unmap a virtual page
void brights_paging_unmap(uint64_t virt);

// Get physical address for virtual address
uint64_t brights_paging_virt_to_phys(uint64_t virt);

// Flush TLB for specific address
void brights_paging_flush_tlb(uint64_t virt);

// Flush entire TLB
void brights_paging_flush_tlb_all(void);

// Get current CR3 value
uint64_t brights_paging_get_cr3(void);

// Set CR3 value
void brights_paging_set_cr3(uint64_t cr3);

#endif
