#include "paging.h"
#include <stdint.h>

// Forward declaration for pmem alloc
extern void *brights_pmem_alloc_page(void);

// Page table entry type
typedef uint64_t pte_t;

// Page table levels
#define PT_LEVEL_PML4 3
#define PT_LEVEL_PDPT 2
#define PT_LEVEL_PD   1
#define PT_LEVEL_PT   0

// Number of entries per page table
#define PT_ENTRIES 512

// Shift for each level
static const int pt_shift[] = {12, 21, 30, 39};

// Root page table (PML4)
static pte_t *pml4_table = 0;

// Get CR3 value
uint64_t brights_paging_get_cr3(void)
{
  uint64_t cr3;
  __asm__ __volatile__("mov %%cr3, %0" : "=r"(cr3));
  return cr3;
}

// Set CR3 value
void brights_paging_set_cr3(uint64_t cr3)
{
  __asm__ __volatile__("mov %0, %%cr3" :: "r"(cr3) : "memory");
}

// Allocate a zeroed page for page table
static pte_t *alloc_pt_page(void)
{
  void *page = brights_pmem_alloc_page();
  if (!page) {
    return 0;
  }
  // Zero the page
  uint8_t *p = (uint8_t *)page;
  for (int i = 0; i < BRIGHTS_PAGE_SIZE_4K; ++i) {
    p[i] = 0;
  }
  return (pte_t *)page;
}

// Get page table entry at specified level
static pte_t *get_pte(pte_t *table, uint64_t virt, int level)
{
  int shift = pt_shift[level];
  int idx = (virt >> shift) & 0x1FF;
  return &table[idx];
}

// Walk page tables, optionally creating missing levels
static pte_t *walk_page_table(uint64_t virt, int create)
{
  if (!pml4_table) {
    return 0;
  }

  pte_t *table = pml4_table;
  
  for (int level = PT_LEVEL_PML4; level >= PT_LEVEL_PT; --level) {
    pte_t *pte = get_pte(table, virt, level);
    
    if (!(*pte & BRIGHTS_PTE_PRESENT)) {
      if (!create) {
        return 0;
      }
      
      // Allocate new page table
      pte_t *new_table = alloc_pt_page();
      if (!new_table) {
        return 0;
      }
      
      *pte = (uint64_t)new_table | BRIGHTS_PTE_PRESENT | BRIGHTS_PTE_WRITABLE;
    }
    
    if (level == PT_LEVEL_PT) {
      return pte;
    }
    
    // Get next level table
    table = (pte_t *)(uintptr_t)(*pte & ~0xFFFULL);
  }
  
  return 0;
}

// Map a virtual page to a physical page
int brights_paging_map(uint64_t virt, uint64_t phys, uint64_t flags)
{
  if (!pml4_table) {
    return -1;
  }
  
  // Align addresses to page boundary
  virt &= ~0xFFFULL;
  phys &= ~0xFFFULL;
  
  pte_t *pte = walk_page_table(virt, 1);
  if (!pte) {
    return -1;
  }
  
  *pte = phys | BRIGHTS_PTE_PRESENT | flags;
  brights_paging_flush_tlb(virt);
  
  return 0;
}

// Unmap a virtual page
void brights_paging_unmap(uint64_t virt)
{
  if (!pml4_table) {
    return;
  }
  
  virt &= ~0xFFFULL;
  
  pte_t *pte = walk_page_table(virt, 0);
  if (pte && (*pte & BRIGHTS_PTE_PRESENT)) {
    *pte = 0;
    brights_paging_flush_tlb(virt);
  }
}

// Get physical address for virtual address
uint64_t brights_paging_virt_to_phys(uint64_t virt)
{
  if (!pml4_table) {
    return 0;
  }
  
  pte_t *pte = walk_page_table(virt, 0);
  if (!pte || !(*pte & BRIGHTS_PTE_PRESENT)) {
    return 0;
  }
  
  return (*pte & ~0xFFFULL) | (virt & 0xFFFULL);
}

// Flush TLB for specific address
void brights_paging_flush_tlb(uint64_t virt)
{
  __asm__ __volatile__("invlpg (%0)" :: "r"(virt) : "memory");
}

// Flush entire TLB
void brights_paging_flush_tlb_all(void)
{
  uint64_t cr3 = brights_paging_get_cr3();
  brights_paging_set_cr3(cr3);
}

// Initialize paging subsystem
void brights_paging_init(void)
{
  // Get current CR3 (UEFI sets up identity mapping)
  uint64_t cr3 = brights_paging_get_cr3();
  pml4_table = (pte_t *)(uintptr_t)(cr3 & ~0xFFFULL);
}
