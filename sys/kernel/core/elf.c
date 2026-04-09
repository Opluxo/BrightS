#include "../core/kernel_util.h"
#include "elf.h"
#include "pmem.h"
#include "../platform/x86_64/paging.h"
#include "../dev/serial.h"

/* User stack: 16 pages = 64KB */
#define USER_STACK_PAGES 16
#define USER_STACK_TOP   0x00007FFFFFFFFFFFULL  /* Canonical high user address */

static void elf_debug(const char *msg)
{
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, msg);
}

static void elf_debug_hex64(uint64_t v)
{
  static const char hex[] = "0123456789ABCDEF";
  char buf[19];
  buf[0] = '0'; buf[1] = 'x';
  for (int i = 0; i < 16; ++i) {
    buf[2 + i] = hex[(v >> ((15 - i) * 4)) & 0xF];
  }
  buf[18] = 0;
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, buf);
}

static uint64_t align_down_4k(uint64_t v)
{
  return v & ~0xFFFULL;
}

static uint64_t align_up_4k(uint64_t v)
{
  return (v + 0xFFF) & ~0xFFFULL;
}

int brights_elf_load(const void *buf, uint64_t size, elf64_load_info_t *info_out)
{
  if (!buf || size < sizeof(elf64_ehdr_t) || !info_out) {
    elf_debug("elf: bad args\r\n");
    return -1;
  }

  const elf64_ehdr_t *ehdr = (const elf64_ehdr_t *)buf;

  /* Validate magic */
  if (ehdr->e_ident[0] != 0x7F || ehdr->e_ident[1] != 'E' ||
      ehdr->e_ident[2] != 'L' || ehdr->e_ident[3] != 'F') {
    elf_debug("elf: bad magic\r\n");
    return -1;
  }

  /* Must be 64-bit LE x86_64 */
  if (ehdr->e_ident[4] != ELF_CLASS_64 || ehdr->e_ident[5] != ELF_DATA_LE ||
      ehdr->e_machine != ELF_MACH_X86_64) {
    elf_debug("elf: wrong arch\r\n");
    return -1;
  }

  /* Must be executable or shared object */
  if (ehdr->e_type != ELF_TYPE_EXEC && ehdr->e_type != ELF_TYPE_DYN) {
    elf_debug("elf: not executable\r\n");
    return -1;
  }

  /* Entry point must be non-zero */
  if (ehdr->e_entry == 0) {
    elf_debug("elf: no entry\r\n");
    return -1;
  }

  /* Validate program header table */
  if (ehdr->e_phoff == 0 || ehdr->e_phnum == 0 || ehdr->e_phentsize != sizeof(elf64_phdr_t)) {
    elf_debug("elf: bad phdr\r\n");
    return -1;
  }

  if (ehdr->e_phoff + (uint64_t)ehdr->e_phnum * ehdr->e_phentsize > size) {
    elf_debug("elf: phdr out of range\r\n");
    return -1;
  }

  const elf64_phdr_t *phdrs = (const elf64_phdr_t *)((const uint8_t *)buf + ehdr->e_phoff);

  /* Find max virtual address to place user stack above segments */
  uint64_t max_vaddr = 0;

  /* First pass: find max vaddr */
  for (uint16_t i = 0; i < ehdr->e_phnum; ++i) {
    if (phdrs[i].p_type != PT_LOAD) continue;
    uint64_t seg_end = phdrs[i].p_vaddr + phdrs[i].p_memsz;
    if (seg_end > max_vaddr) max_vaddr = seg_end;
  }

  /* Second pass: map PT_LOAD segments */
  for (uint16_t i = 0; i < ehdr->e_phnum; ++i) {
    if (phdrs[i].p_type != PT_LOAD) continue;

    uint64_t seg_vaddr = phdrs[i].p_vaddr;
    uint64_t seg_filesz = phdrs[i].p_filesz;
    uint64_t seg_memsz = phdrs[i].p_memsz;

    if (seg_memsz == 0) continue;

    /* File data must be within bounds */
    if (phdrs[i].p_offset + seg_filesz > size) {
      elf_debug("elf: segment out of range\r\n");
      return -1;
    }

    uint64_t page_start = align_down_4k(seg_vaddr);
    uint64_t page_end = align_up_4k(seg_vaddr + seg_memsz);
    uint64_t num_pages = (page_end - page_start) / 4096;

    elf_debug("elf: mapping ");
    elf_debug_hex64(num_pages);
    elf_debug(" pages at ");
    elf_debug_hex64(page_start);
    elf_debug("\r\n");

    /* Determine page flags */
    uint64_t pte_flags = BRIGHTS_PTE_PRESENT | BRIGHTS_PTE_USER;
    if (phdrs[i].p_flags & PF_W) {
      pte_flags |= BRIGHTS_PTE_WRITABLE;
    }

    /* Allocate and map pages, copy data */
    for (uint64_t pg = 0; pg < num_pages; ++pg) {
      uint64_t vaddr = page_start + pg * 4096;
      void *phys_page = brights_pmem_alloc_page();
      if (!phys_page) {
        elf_debug("elf: pmem alloc fail\r\n");
        return -1;
      }

      /* Zero the page first */
      kutil_memset(phys_page, 0, 4096);

      /* Copy file data for this page */
      uint64_t page_copy_len = 0;

      if (seg_filesz > 0) {
        /* How many bytes of this page overlap with file data? */
        uint64_t file_offset_for_page = phdrs[i].p_offset + pg * 4096;
        if (pg == 0) {
          file_offset_for_page = phdrs[i].p_offset;
        }

        /* Bytes of file data in this page */
        uint64_t file_bytes_remaining = seg_filesz - pg * 4096;
        if (pg == 0) {
          file_bytes_remaining = seg_filesz;
        }

        /* Simpler: compute how much to copy into this page */
        uint64_t copy_start_vaddr = (seg_vaddr > vaddr) ? seg_vaddr : vaddr;
        uint64_t copy_end_vaddr = vaddr + 4096;
        uint64_t seg_data_end = seg_vaddr + seg_filesz;
        if (seg_data_end < copy_end_vaddr) copy_end_vaddr = seg_data_end;

        if (copy_start_vaddr < copy_end_vaddr) {
          page_copy_len = copy_end_vaddr - copy_start_vaddr;
          uint64_t src_off = phdrs[i].p_offset + (copy_start_vaddr - seg_vaddr);
          uint64_t dst_off = copy_start_vaddr - vaddr;
          kutil_memcpy((uint8_t *)phys_page + dst_off,
                       (const uint8_t *)buf + src_off, page_copy_len);
        }
      }

      /* Map the page */
      if (brights_paging_map(vaddr, (uint64_t)(uintptr_t)phys_page, pte_flags) != 0) {
        elf_debug("elf: paging map fail\r\n");
        return -1;
      }
    }
  }

  /* Map program headers into user space so the process can access them */
  uint64_t phdr_pages = align_up_4k(ehdr->e_phnum * sizeof(elf64_phdr_t)) / 4096;
  if (phdr_pages == 0) phdr_pages = 1;
  void *phdr_phys = brights_pmem_alloc_page();
  if (!phdr_phys) {
    elf_debug("elf: phdr alloc fail\r\n");
    return -1;
  }
  kutil_memset(phdr_phys, 0, 4096);
  uint64_t phdr_user_vaddr = align_up_4k(max_vaddr);
  if (phdr_user_vaddr < max_vaddr) phdr_user_vaddr = max_vaddr;
  /* Align to next page boundary if not already */
  phdr_user_vaddr = align_up_4k(phdr_user_vaddr);
  kutil_memcpy(phdr_phys, phdrs, ehdr->e_phnum * sizeof(elf64_phdr_t));
  if (brights_paging_map(phdr_user_vaddr, (uint64_t)(uintptr_t)phdr_phys,
                          BRIGHTS_PTE_PRESENT | BRIGHTS_PTE_USER) != 0) {
    elf_debug("elf: phdr map fail\r\n");
    return -1;
  }

  /* Set up user stack: allocate pages and map below the canonical hole,
     or at a known user address */
  uint64_t stack_base = USER_STACK_TOP - (uint64_t)USER_STACK_PAGES * 4096;
  stack_base = align_down_4k(stack_base);

  for (int i = 0; i < USER_STACK_PAGES; ++i) {
    void *sp = brights_pmem_alloc_page();
    if (!sp) {
      elf_debug("elf: stack alloc fail\r\n");
      return -1;
    }
    kutil_memset(sp, 0, 4096);
    uint64_t sv = stack_base + (uint64_t)i * 4096;
    if (brights_paging_map(sv, (uint64_t)(uintptr_t)sp,
                            BRIGHTS_PTE_PRESENT | BRIGHTS_PTE_WRITABLE | BRIGHTS_PTE_USER) != 0) {
      elf_debug("elf: stack map fail\r\n");
      return -1;
    }
  }

  info_out->entry = ehdr->e_entry;
  info_out->stack_top = stack_base + (uint64_t)USER_STACK_PAGES * 4096;
  info_out->phdr = phdr_user_vaddr;
  info_out->phnum = ehdr->e_phnum;
  info_out->phentsize = ehdr->e_phentsize;

  elf_debug("elf: loaded entry=");
  elf_debug_hex64(info_out->entry);
  elf_debug(" stack=");
  elf_debug_hex64(info_out->stack_top);
  elf_debug("\r\n");

  return 0;
}
