#include "paging.h"
#include "../../kernel/core/pmem.h"
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

int brights_paging_set_caching(uint64_t virt, uint64_t caching_flags)
{
  if (!pml4_table) return -1;
  virt &= ~0xFFFULL;

  /* Walk to find the PTE, including 2MB huge pages at PD level */
  pte_t *table = pml4_table;
  for (int level = PT_LEVEL_PML4; level >= PT_LEVEL_PT; --level) {
    pte_t *pte = get_pte(table, virt, level);
    if (!(*pte & BRIGHTS_PTE_PRESENT)) return -1;

    /* Huge page found — modify flags in-place */
    if (*pte & BRIGHTS_PTE_HUGE) {
      *pte = (*pte & ~(BRIGHTS_PTE_CD | BRIGHTS_PTE_WT)) | (caching_flags & (BRIGHTS_PTE_CD | BRIGHTS_PTE_WT));
      brights_paging_flush_tlb(virt);
      return 0;
    }

    if (level == PT_LEVEL_PT) {
      *pte = (*pte & ~(BRIGHTS_PTE_CD | BRIGHTS_PTE_WT)) | (caching_flags & (BRIGHTS_PTE_CD | BRIGHTS_PTE_WT));
      brights_paging_flush_tlb(virt);
      return 0;
    }
    table = (pte_t *)(uintptr_t)(*pte & ~0xFFFULL);
  }
  return -1;
}

/* Remap a physical address range to a virtual address with uncachable
 * (PCD+PWT) flags by creating new page table pages along the path.
 * Avoids writing to UEFI's existing read-only page table pages.
 * Must be called after pmem is initialized. */
int brights_paging_remap_uc(uint64_t virt, uint64_t phys, uint64_t size)
{
  if (!pml4_table) return -1;

  /* Fork PML4 → new PDPT */
  int pml4_idx = (virt >> 39) & 0x1FF;
  pte_t *old_pml4_e = &pml4_table[pml4_idx];
  if (!(*old_pml4_e & BRIGHTS_PTE_PRESENT)) return -1;

  pte_t *new_pml4 = alloc_pt_page();
  if (!new_pml4) return -1;
  for (int i = 0; i < PT_ENTRIES; i++) new_pml4[i] = pml4_table[i];

  pte_t *new_pdpt = alloc_pt_page();
  if (!new_pdpt) return -1;
  pte_t *old_pdpt = (pte_t *)(uintptr_t)(*old_pml4_e & ~0xFFFULL);
  for (int i = 0; i < PT_ENTRIES; i++) new_pdpt[i] = old_pdpt[i];
  new_pml4[pml4_idx] = (uint64_t)(uintptr_t)new_pdpt |
    (*old_pml4_e & (BRIGHTS_PTE_PRESENT | BRIGHTS_PTE_WRITABLE | BRIGHTS_PTE_USER));

  /* Fill each 2MB PD slice that overlaps the range */
  uint64_t pages = (size + BRIGHTS_PAGE_SIZE_4K - 1) / BRIGHTS_PAGE_SIZE_4K;
  for (uint64_t done = 0; done < pages; ) {
    int pdpt_i = ((virt + done * BRIGHTS_PAGE_SIZE_4K) >> 30) & 0x1FF;
    int pd_i   = ((virt + done * BRIGHTS_PAGE_SIZE_4K) >> 21) & 0x1FF;
    int pt_i   = ((virt + done * BRIGHTS_PAGE_SIZE_4K) >> 12) & 0x1FF;

    /* Fork PDPT entry → new PD (only once per pdpt_i) */
    pte_t *old_pdpt_e = &old_pdpt[pdpt_i];
    if (!(*old_pdpt_e & BRIGHTS_PTE_PRESENT)) return -1;

    pte_t *new_pd;
    if (new_pdpt[pdpt_i] != old_pdpt[pdpt_i]) {
      /* Already forked this PDPT entry — reuse existing new PD */
      new_pd = (pte_t *)(uintptr_t)(new_pdpt[pdpt_i] & ~0xFFFULL);
    } else {
      new_pd = alloc_pt_page();
      if (!new_pd) return -1;
      if (*old_pdpt_e & BRIGHTS_PTE_HUGE) {
        uint64_t base = *old_pdpt_e & ~0x3FFFFFFFULL;
        for (int i = 0; i < PT_ENTRIES; i++)
          new_pd[i] = (base + (uint64_t)i * BRIGHTS_PAGE_SIZE_2M) |
            BRIGHTS_PTE_PRESENT | BRIGHTS_PTE_WRITABLE | BRIGHTS_PTE_HUGE;
      } else {
        pte_t *old_pd = (pte_t *)(uintptr_t)(*old_pdpt_e & ~0xFFFULL);
        for (int i = 0; i < PT_ENTRIES; i++) new_pd[i] = old_pd[i];
      }
      new_pdpt[pdpt_i] = (uint64_t)(uintptr_t)new_pd |
        (BRIGHTS_PTE_PRESENT | BRIGHTS_PTE_WRITABLE);
    }

    /* Fork PD entry → new PT */
    pte_t *old_pd_e = &new_pd[pd_i];
    if (!(*old_pd_e & BRIGHTS_PTE_PRESENT)) return -1;

    pte_t *new_pt = alloc_pt_page();
    if (!new_pt) return -1;
    if (*old_pd_e & BRIGHTS_PTE_HUGE) {
      uint64_t base = *old_pd_e & ~0x1FFFFFULL;
      for (int i = 0; i < PT_ENTRIES; i++)
        new_pt[i] = (base + (uint64_t)i * BRIGHTS_PAGE_SIZE_4K) |
          BRIGHTS_PTE_PRESENT | BRIGHTS_PTE_WRITABLE;
    } else {
      pte_t *old_pt = (pte_t *)(uintptr_t)(*old_pd_e & ~0xFFFULL);
      for (int i = 0; i < PT_ENTRIES; i++) new_pt[i] = old_pt[i];
    }
    new_pd[pd_i] = (uint64_t)(uintptr_t)new_pt |
      (BRIGHTS_PTE_PRESENT | BRIGHTS_PTE_WRITABLE);

    /* Fill PT entries for this 2MB slice */
    uint64_t p = (phys + done * BRIGHTS_PAGE_SIZE_4K) & ~0xFFFULL;
    uint64_t remaining = pages - done;
    uint64_t pt_space = (uint64_t)(PT_ENTRIES - pt_i);
    uint64_t count = (remaining < pt_space) ? remaining : pt_space;
    for (uint64_t j = 0; j < count; j++) {
      new_pt[pt_i + (int)j] = (p + j * BRIGHTS_PAGE_SIZE_4K) |
        BRIGHTS_PTE_PRESENT | BRIGHTS_PTE_WRITABLE | BRIGHTS_PTE_CD | BRIGHTS_PTE_WT;
    }
    done += count;
  }

  /* Switch to new PML4 */
  pml4_table = new_pml4;
  uint64_t new_cr3 = (uint64_t)(uintptr_t)pml4_table;
  __asm__ __volatile__("mov %0, %%cr3" :: "r"(new_cr3) : "memory");
  return 0;
}
