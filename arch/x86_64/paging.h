#ifndef BRIGHTS_X86_64_PAGING_H
#define BRIGHTS_X86_64_PAGING_H

#include <stdint.h>

/* Page size constants */
#define BRIGHTS_PAGE_SIZE_4K 4096ULL
#define BRIGHTS_PAGE_SIZE_2M (2ULL * 1024 * 1024)
#define BRIGHTS_PAGE_SIZE_1G (1ULL * 1024 * 1024 * 1024)

/* Alias for common usage */
#define BRIGHTS_PAGE_SIZE BRIGHTS_PAGE_SIZE_4K

/* Page table entry flags */
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

/* PAT index bits in PTE (for custom caching via PAT MSR) */
#define BRIGHTS_PTE_PAT       (1ULL << 7)  /* Same as HUGE for PT level, different for PD */
#define BRIGHTS_PTE_PCD       (1ULL << 4)
#define BRIGHTS_PTE_PWT       (1ULL << 3)

/* Initialize paging subsystem */
void brights_paging_init(void);

/* Map a 4KB virtual page to a physical page */
int brights_paging_map(uint64_t virt, uint64_t phys, uint64_t flags);

/* Map a 2MB virtual page to a physical 2MB-aligned page */
int brights_paging_map_2m(uint64_t virt, uint64_t phys, uint64_t flags);

/* Unmap a virtual page (4KB or 2MB) */
void brights_paging_unmap(uint64_t virt);

/* Unmap a 2MB page specifically */
void brights_paging_unmap_2m(uint64_t virt);

/* Get physical address for virtual address */
uint64_t brights_paging_virt_to_phys(uint64_t virt);

/* Flush TLB for specific address */
void brights_paging_flush_tlb(uint64_t virt);

/* Flush entire TLB */
void brights_paging_flush_tlb_all(void);

/* Get current CR3 value */
uint64_t brights_paging_get_cr3(void);

/* Set CR3 value */
void brights_paging_set_cr3(uint64_t cr3);

/* Create a new empty page table (PML4) for a process */
uint64_t brights_paging_create_address_space(void);

/* Destroy a page table and free all page table pages */
void brights_paging_destroy_address_space(uint64_t cr3);

/* Clone address space (for fork) */
uint64_t brights_paging_clone_address_space(uint64_t src_cr3);

/* Mark a page as writable (for COW resolution) */
int brights_paging_mark_writable(uint64_t virt);

#endif
