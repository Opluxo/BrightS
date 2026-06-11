#ifndef BRIGHTS_I386_PAGING_H
#define BRIGHTS_I386_PAGING_H

#include <stdint.h>

#define BRIGHTS_PAGE_SIZE_4K 4096UL
#define BRIGHTS_PAGE_SIZE_4M (4UL * 1024 * 1024)
#define BRIGHTS_PAGE_SIZE BRIGHTS_PAGE_SIZE_4K

#define BRIGHTS_PTE_PRESENT   (1UL << 0)
#define BRIGHTS_PTE_WRITABLE  (1UL << 1)
#define BRIGHTS_PTE_USER      (1UL << 2)
#define BRIGHTS_PTE_WT        (1UL << 3)
#define BRIGHTS_PTE_CD        (1UL << 4)
#define BRIGHTS_PTE_ACCESSED  (1UL << 5)
#define BRIGHTS_PTE_DIRTY     (1UL << 6)
#define BRIGHTS_PTE_HUGE      (1UL << 7)
#define BRIGHTS_PTE_GLOBAL    (1UL << 8)
#define BRIGHTS_PTE_PAT       (1UL << 7)
#define BRIGHTS_PTE_PCD       (1UL << 4)
#define BRIGHTS_PTE_PWT       (1UL << 3)

void brights_paging_init(void);
void brights_paging_enable(void);
int brights_paging_map(uint32_t virt, uint32_t phys, uint32_t flags);
int brights_paging_map_4m(uint32_t virt, uint32_t phys, uint32_t flags);
void brights_paging_unmap(uint32_t virt);
uint32_t brights_paging_virt_to_phys(uint32_t virt);
void brights_paging_flush_tlb(uint32_t virt);
void brights_paging_flush_tlb_all(void);
uint32_t brights_paging_get_cr3(void);
void brights_paging_set_cr3(uint32_t cr3);
uint32_t brights_paging_create_address_space(void);
void brights_paging_destroy_address_space(uint32_t cr3);
uint32_t brights_paging_clone_address_space(uint32_t src_cr3);
int brights_paging_mark_writable(uint32_t virt);

#endif
