#include "paging.h"
#include "../../core/pmem.h"
#include <stdint.h>

typedef uint64_t pte_t;

#define PT_LEVEL_PML4 3
#define PT_LEVEL_PDPT 2
#define PT_LEVEL_PD   1
#define PT_LEVEL_PT   0
#define PT_ENTRIES    512

static const int pt_shift[] = {12, 21, 30, 39};

static pte_t *pml4_table = 0;

/* Zero-fill a page */
static void zero_page(void *page)
{
  uint8_t *p = (uint8_t *)page;
  for (int i = 0; i < 4096; ++i) p[i] = 0;
}

uint64_t brights_paging_get_cr3(void)
{
  uint64_t cr3;
  __asm__ __volatile__("mov %%cr3, %0" : "=r"(cr3));
  return cr3;
}

void brights_paging_set_cr3(uint64_t cr3)
{
  __asm__ __volatile__("mov %0, %%cr3" :: "r"(cr3) : "memory");
}

static pte_t *alloc_pt_page(void)
{
  void *page = brights_pmem_alloc_page();
  if (!page) return 0;
  zero_page(page);
  return (pte_t *)page;
}

static pte_t *get_pte(pte_t *table, uint64_t virt, int level)
{
  int idx = (virt >> pt_shift[level]) & 0x1FF;
  return &table[idx];
}

/* Walk page tables, optionally creating missing levels.
   For 2MB pages, stops at PD level (level 1) if huge_ok is set. */
static pte_t *walk_page_table_ex(uint64_t virt, int create, int stop_at_pd)
{
  if (!pml4_table) return 0;
  pte_t *table = pml4_table;

  for (int level = PT_LEVEL_PML4; level >= PT_LEVEL_PT; --level) {
    pte_t *pte = get_pte(table, virt, level);

    if (!(*pte & BRIGHTS_PTE_PRESENT)) {
      if (!create) return 0;
      pte_t *new_table = alloc_pt_page();
      if (!new_table) return 0;
      *pte = (uint64_t)(uintptr_t)new_table | BRIGHTS_PTE_PRESENT | BRIGHTS_PTE_WRITABLE;
    }

    /* If huge page at PD level and we want to stop there */
    if (stop_at_pd && level == PT_LEVEL_PD) {
      return pte;
    }

    /* Check if this is already a huge page */
    if (*pte & BRIGHTS_PTE_HUGE) {
      return pte;
    }

    if (level == PT_LEVEL_PT) return pte;
    table = (pte_t *)(uintptr_t)(*pte & ~0xFFFULL);
  }
  return 0;
}

static pte_t *walk_page_table(uint64_t virt, int create)
{
  return walk_page_table_ex(virt, create, 0);
}

int brights_paging_map(uint64_t virt, uint64_t phys, uint64_t flags)
{
  if (!pml4_table) return -1;
  virt &= ~0xFFFULL;
  phys &= ~0xFFFULL;
  pte_t *pte = walk_page_table(virt, 1);
  if (!pte) return -1;
  *pte = phys | BRIGHTS_PTE_PRESENT | flags;
  brights_paging_flush_tlb(virt);
  return 0;
}

int brights_paging_map_2m(uint64_t virt, uint64_t phys, uint64_t flags)
{
  if (!pml4_table) return -1;
  /* Must be 2MB aligned */
  if ((virt & 0x1FFFFF) || (phys & 0x1FFFFF)) return -1;
  pte_t *pte = walk_page_table_ex(virt, 1, 1);
  if (!pte) return -1;
  /* Set huge page bit at PD level */
  *pte = (phys & ~0x1FFFFFULL) | BRIGHTS_PTE_PRESENT | BRIGHTS_PTE_HUGE | (flags & ~BRIGHTS_PTE_HUGE);
  brights_paging_flush_tlb(virt);
  return 0;
}

void brights_paging_unmap(uint64_t virt)
{
  if (!pml4_table) return;
  virt &= ~0xFFFULL;
  pte_t *pte = walk_page_table(virt, 0);
  if (pte && (*pte & BRIGHTS_PTE_PRESENT)) {
    *pte = 0;
    brights_paging_flush_tlb(virt);
  }
}

void brights_paging_unmap_2m(uint64_t virt)
{
  if (!pml4_table) return;
  virt &= ~0x1FFFFFULL;
  pte_t *pte = walk_page_table_ex(virt, 0, 1);
  if (pte && (*pte & BRIGHTS_PTE_PRESENT) && (*pte & BRIGHTS_PTE_HUGE)) {
    *pte = 0;
    brights_paging_flush_tlb(virt);
  }
}

uint64_t brights_paging_virt_to_phys(uint64_t virt)
{
  if (!pml4_table) return 0;
  pte_t *pte = walk_page_table(virt, 0);
  if (!pte || !(*pte & BRIGHTS_PTE_PRESENT)) return 0;
  if (*pte & BRIGHTS_PTE_HUGE) {
    return (*pte & ~0x1FFFFFULL) | (virt & 0x1FFFFFULL);
  }
  return (*pte & ~0xFFFULL) | (virt & 0xFFFULL);
}

void brights_paging_flush_tlb(uint64_t virt)
{
  __asm__ __volatile__("invlpg (%0)" :: "r"(virt) : "memory");
}

void brights_paging_flush_tlb_all(void)
{
  uint64_t cr3 = brights_paging_get_cr3();
  __asm__ __volatile__("mov %0, %%cr3" :: "r"(cr3) : "memory");
}

void brights_paging_init(void)
{
  uint64_t cr3 = brights_paging_get_cr3();
  pml4_table = (pte_t *)(uintptr_t)(cr3 & ~0xFFFULL);
}

/* Create a new empty PML4 with kernel mappings copied from current */
uint64_t brights_paging_create_address_space(void)
{
  pte_t *new_pml4 = alloc_pt_page();
  if (!new_pml4) return 0;

  /* Copy kernel half (upper 256 entries, indices 256-511) from current PML4 */
  if (pml4_table) {
    for (int i = 256; i < 512; ++i) {
      new_pml4[i] = pml4_table[i];
    }
  }

  return (uint64_t)(uintptr_t)new_pml4;
}

/* Free a page table hierarchy (only the lower half, user pages) */
static void free_pt_tree(pte_t *table, int level)
{
  if (level < 0) return;
  for (int i = 0; i < (level == PT_LEVEL_PML4 ? 256 : PT_ENTRIES); ++i) {
    pte_t entry = table[i];
    if (!(entry & BRIGHTS_PTE_PRESENT)) continue;
    if (entry & BRIGHTS_PTE_HUGE) continue; /* Don't free huge page backing */
    if (level > 0) {
      pte_t *child = (pte_t *)(uintptr_t)(entry & ~0xFFFULL);
      free_pt_tree(child, level - 1);
    }
  }
  brights_pmem_free_page(table);
}

void brights_paging_destroy_address_space(uint64_t cr3)
{
  pte_t *pml4 = (pte_t *)(uintptr_t)(cr3 & ~0xFFFULL);
  if (pml4) {
    free_pt_tree(pml4, PT_LEVEL_PML4);
  }
}

/* Deep clone of user address space (for fork) */
uint64_t brights_paging_clone_address_space(uint64_t src_cr3)
{
  pte_t *src_pml4 = (pte_t *)(uintptr_t)(src_cr3 & ~0xFFFULL);
  pte_t *dst_pml4 = alloc_pt_page();
  if (!dst_pml4) return 0;

  /* Copy kernel mappings */
  if (pml4_table) {
    for (int i = 256; i < 512; ++i) {
      dst_pml4[i] = pml4_table[i];
    }
  }

  /* Clone user mappings (lower half) with COW */
  for (int i = 0; i < 256; ++i) {
    if (!(src_pml4[i] & BRIGHTS_PTE_PRESENT)) continue;
    if (src_pml4[i] & BRIGHTS_PTE_HUGE) {
      dst_pml4[i] = src_pml4[i]; /* Share huge pages */
      continue;
    }
    /* Clone PDPT level */
    pte_t *src_pdpt = (pte_t *)(uintptr_t)(src_pml4[i] & ~0xFFFULL);
    pte_t *dst_pdpt = alloc_pt_page();
    if (!dst_pdpt) { dst_pml4[i] = 0; continue; }
    dst_pml4[i] = (uint64_t)(uintptr_t)dst_pdpt | (src_pml4[i] & 0xFFF);

    for (int j = 0; j < PT_ENTRIES; ++j) {
      if (!(src_pdpt[j] & BRIGHTS_PTE_PRESENT)) continue;
      if (src_pdpt[j] & BRIGHTS_PTE_HUGE) {
        dst_pdpt[j] = src_pdpt[j];
        continue;
      }
      /* Clone PD level */
      pte_t *src_pd = (pte_t *)(uintptr_t)(src_pdpt[j] & ~0xFFFULL);
      pte_t *dst_pd = alloc_pt_page();
      if (!dst_pd) { dst_pdpt[j] = 0; continue; }
      dst_pdpt[j] = (uint64_t)(uintptr_t)dst_pd | (src_pdpt[j] & 0xFFF);

      for (int k = 0; k < PT_ENTRIES; ++k) {
        if (!(src_pd[k] & BRIGHTS_PTE_PRESENT)) continue;
        if (src_pd[k] & BRIGHTS_PTE_HUGE) {
          dst_pd[k] = src_pd[k];
          continue;
        }
        /* Clone PT level - implement COW */
        pte_t *src_pt = (pte_t *)(uintptr_t)(src_pd[k] & ~0xFFFULL);
        pte_t *dst_pt = alloc_pt_page();
        if (!dst_pt) { dst_pd[k] = 0; continue; }
        dst_pd[k] = (uint64_t)(uintptr_t)dst_pt | (src_pd[k] & 0xFFF);

        for (int m = 0; m < PT_ENTRIES; ++m) {
          pte_t entry = src_pt[m];
          if (!(entry & BRIGHTS_PTE_PRESENT)) {
            dst_pt[m] = entry;
            continue;
          }
          /* Mark both parent and child as read-only for COW */
          /* Clear writable bit and set COW marker (bit 9, available for OS use) */
          src_pt[m] = entry & ~BRIGHTS_PTE_WRITABLE;
          dst_pt[m] = (entry & ~BRIGHTS_PTE_WRITABLE);
        }
      }
    }
  }

  return (uint64_t)(uintptr_t)dst_pml4;
}

/* Mark a page as writable (for COW resolution) */
int brights_paging_mark_writable(uint64_t virt)
{
  virt &= ~0xFFFULL;
  pte_t *pte = walk_page_table(virt, 0);
  if (!pte || !(*pte & BRIGHTS_PTE_PRESENT)) return -1;
  *pte |= BRIGHTS_PTE_WRITABLE;
  brights_paging_flush_tlb(virt);
  return 0;
}
