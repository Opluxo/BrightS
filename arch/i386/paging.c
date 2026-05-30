#include "paging.h"
#include "../../kernel/core/pmem.h"
#include <stdint.h>

typedef uint32_t pte_t;

#define PD_ENTRIES 1024
#define PT_ENTRIES 1024

static pte_t *page_directory = 0;

static void zero_page(void *page)
{
  uint8_t *p = (uint8_t *)page;
  for (int i = 0; i < 4096; ++i) p[i] = 0;
}

uint32_t brights_paging_get_cr3(void)
{
  uint32_t cr3;
  __asm__ __volatile__("mov %%cr3, %0" : "=r"(cr3));
  return cr3;
}

void brights_paging_set_cr3(uint32_t cr3)
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

int brights_paging_map(uint32_t virt, uint32_t phys, uint32_t flags)
{
  if (!page_directory) return -1;
  virt &= ~0xFFF;
  phys &= ~0xFFF;
  uint32_t pd_idx = virt >> 22;
  uint32_t pt_idx = (virt >> 12) & 0x3FF;

  if (!(page_directory[pd_idx] & BRIGHTS_PTE_PRESENT))
  {
    pte_t *pt = alloc_pt_page();
    if (!pt) return -1;
    page_directory[pd_idx] = (uint32_t)(uintptr_t)pt | BRIGHTS_PTE_PRESENT | BRIGHTS_PTE_WRITABLE;
  }

  pte_t *page_table = (pte_t *)(uintptr_t)(page_directory[pd_idx] & ~0xFFF);
  page_table[pt_idx] = phys | BRIGHTS_PTE_PRESENT | flags;
  brights_paging_flush_tlb(virt);
  return 0;
}

int brights_paging_map_4m(uint32_t virt, uint32_t phys, uint32_t flags)
{
  if (!page_directory) return -1;
  if ((virt & 0x3FFFFF) || (phys & 0x3FFFFF)) return -1;
  uint32_t pd_idx = virt >> 22;
  page_directory[pd_idx] = (phys & ~0x3FFFFF) | BRIGHTS_PTE_PRESENT | BRIGHTS_PTE_HUGE | flags;
  brights_paging_flush_tlb(virt);
  return 0;
}

void brights_paging_unmap(uint32_t virt)
{
  if (!page_directory) return;
  uint32_t pd_idx = virt >> 22;
  uint32_t pt_idx = (virt >> 12) & 0x3FF;
  if (!(page_directory[pd_idx] & BRIGHTS_PTE_PRESENT)) return;
  pte_t *page_table = (pte_t *)(uintptr_t)(page_directory[pd_idx] & ~0xFFF);
  if (page_table[pt_idx] & BRIGHTS_PTE_PRESENT)
  {
    page_table[pt_idx] = 0;
    brights_paging_flush_tlb(virt);
  }
}

uint32_t brights_paging_virt_to_phys(uint32_t virt)
{
  if (!page_directory) return 0;
  uint32_t pd_idx = virt >> 22;
  uint32_t pt_idx = (virt >> 12) & 0x3FF;
  if (!(page_directory[pd_idx] & BRIGHTS_PTE_PRESENT)) return 0;
  if (page_directory[pd_idx] & BRIGHTS_PTE_HUGE)
  {
    return (page_directory[pd_idx] & ~0x3FFFFF) | (virt & 0x3FFFFF);
  }
  pte_t *page_table = (pte_t *)(uintptr_t)(page_directory[pd_idx] & ~0xFFF);
  if (!(page_table[pt_idx] & BRIGHTS_PTE_PRESENT)) return 0;
  return (page_table[pt_idx] & ~0xFFF) | (virt & 0xFFF);
}

void brights_paging_flush_tlb(uint32_t virt)
{
  __asm__ __volatile__("invlpg (%0)" :: "r"(virt) : "memory");
}

void brights_paging_flush_tlb_all(void)
{
  uint32_t cr3 = brights_paging_get_cr3();
  __asm__ __volatile__("mov %0, %%cr3" :: "r"(cr3) : "memory");
}

void brights_paging_init(void)
{
  uint32_t cr3 = brights_paging_get_cr3();
  page_directory = (pte_t *)(uintptr_t)(cr3 & ~0xFFF);
}

void brights_paging_enable(void)
{
  static pte_t initial_pd[PD_ENTRIES] __attribute__((aligned(4096)));
  static pte_t initial_pt[4][PT_ENTRIES] __attribute__((aligned(4096)));
  uint32_t flags = BRIGHTS_PTE_PRESENT | BRIGHTS_PTE_WRITABLE;

  for (int pt_idx = 0; pt_idx < 4; ++pt_idx)
  {
    for (int i = 0; i < PT_ENTRIES; ++i)
    {
      initial_pt[pt_idx][i] = ((pt_idx << 22) | (i << 12)) | flags;
    }
    initial_pd[pt_idx] = (uint32_t)(uintptr_t)initial_pt[pt_idx] | flags;
  }
  for (int i = 4; i < PD_ENTRIES; ++i)
  {
    initial_pd[i] = 0;
  }

  __asm__ __volatile__("mov %0, %%cr3" : : "r"((uint32_t)(uintptr_t)initial_pd));

  uint32_t cr0;
  __asm__ __volatile__("mov %%cr0, %0" : "=r"(cr0));
  cr0 |= 0x80000000;
  __asm__ __volatile__("mov %0, %%cr0" : : "r"(cr0));

  page_directory = initial_pd;
}

uint32_t brights_paging_create_address_space(void)
{
  pte_t *new_pd = alloc_pt_page();
  if (!new_pd) return 0;
  if (page_directory)
  {
    for (int i = 768; i < 1024; ++i)
    {
      new_pd[i] = page_directory[i];
    }
  }
  return (uint32_t)(uintptr_t)new_pd;
}

static void free_pt_tree(pte_t *pd, int level)
{
  if (level < 0) return;
  for (int i = 0; i < (level == 0 ? 768 : PD_ENTRIES); ++i)
  {
    pte_t entry = pd[i];
    if (!(entry & BRIGHTS_PTE_PRESENT)) continue;
    if (entry & BRIGHTS_PTE_HUGE) continue;
    if (level > 0)
    {
      pte_t *child = (pte_t *)(uintptr_t)(entry & ~0xFFF);
      free_pt_tree(child, level - 1);
    }
  }
  brights_pmem_free_page(pd);
}

void brights_paging_destroy_address_space(uint32_t cr3)
{
  pte_t *pd = (pte_t *)(uintptr_t)(cr3 & ~0xFFF);
  if (pd)
  {
    free_pt_tree(pd, 1);
  }
}

uint32_t brights_paging_clone_address_space(uint32_t src_cr3)
{
  pte_t *src_pd = (pte_t *)(uintptr_t)(src_cr3 & ~0xFFF);
  pte_t *dst_pd = alloc_pt_page();
  if (!dst_pd) return 0;

  if (page_directory)
  {
    for (int i = 768; i < 1024; ++i)
    {
      dst_pd[i] = page_directory[i];
    }
  }

  for (int i = 0; i < 768; ++i)
  {
    if (!(src_pd[i] & BRIGHTS_PTE_PRESENT)) continue;
    if (src_pd[i] & BRIGHTS_PTE_HUGE)
    {
      dst_pd[i] = src_pd[i];
      continue;
    }
    pte_t *src_pt = (pte_t *)(uintptr_t)(src_pd[i] & ~0xFFF);
    pte_t *dst_pt = alloc_pt_page();
    if (!dst_pt) { dst_pd[i] = 0; continue; }
    dst_pd[i] = (uint32_t)(uintptr_t)dst_pt | (src_pd[i] & 0xFFF);
    for (int j = 0; j < PT_ENTRIES; ++j)
    {
      pte_t entry = src_pt[j];
      if (!(entry & BRIGHTS_PTE_PRESENT))
      {
        dst_pt[j] = entry;
        continue;
      }
      src_pt[j] = entry & ~BRIGHTS_PTE_WRITABLE;
      dst_pt[j] = entry & ~BRIGHTS_PTE_WRITABLE;
    }
  }
  return (uint32_t)(uintptr_t)dst_pd;
}

int brights_paging_mark_writable(uint32_t virt)
{
  virt &= ~0xFFF;
  uint32_t pd_idx = virt >> 22;
  uint32_t pt_idx = (virt >> 12) & 0x3FF;
  if (!page_directory) return -1;
  if (!(page_directory[pd_idx] & BRIGHTS_PTE_PRESENT)) return -1;
  pte_t *page_table = (pte_t *)(uintptr_t)(page_directory[pd_idx] & ~0xFFF);
  if (!(page_table[pt_idx] & BRIGHTS_PTE_PRESENT)) return -1;
  page_table[pt_idx] |= BRIGHTS_PTE_WRITABLE;
  brights_paging_flush_tlb(virt);
  return 0;
}
