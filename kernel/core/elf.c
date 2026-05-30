#include "kernel_util.h"
#include "elf.h"
#include "pmem.h"
#ifdef __i386__
#include "../arch/i386/paging.h"
#else
#include "../arch/x86_64/paging.h"
#endif
#include "../drivers/serial.h"

/* User stack: 16 pages = 64KB */
#define USER_STACK_PAGES 16

/* 64-bit user stack top (canonical high user address) */
#define USER_STACK_TOP_64   0x00007FFFFFFFFFFFULL
/* 32-bit user stack top (below 3GB boundary) */
#define USER_STACK_TOP_32   0xBF800000u

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

/* ---- ELF64 loader ---- */
static int elf64_load(const void *buf, uint64_t size, elf64_load_info_t *info_out)
{
  if (!buf || size < sizeof(elf64_ehdr_t) || !info_out) {
    elf_debug("elf: bad args\r\n");
    return -1;
  }

  const elf64_ehdr_t *ehdr = (const elf64_ehdr_t *)buf;

  if (ehdr->e_ident[0] != 0x7F || ehdr->e_ident[1] != 'E' ||
      ehdr->e_ident[2] != 'L' || ehdr->e_ident[3] != 'F') {
    elf_debug("elf: bad magic\r\n");
    return -1;
  }

  if (ehdr->e_ident[4] != ELF_CLASS_64 || ehdr->e_ident[5] != ELF_DATA_LE ||
      ehdr->e_machine != ELF_MACH_X86_64) {
    elf_debug("elf: wrong arch (64-bit)\r\n");
    return -1;
  }

  if (ehdr->e_type != ELF_TYPE_EXEC && ehdr->e_type != ELF_TYPE_DYN) {
    elf_debug("elf: not executable\r\n");
    return -1;
  }

  if (ehdr->e_entry == 0) {
    elf_debug("elf: no entry\r\n");
    return -1;
  }

  if (ehdr->e_phoff == 0 || ehdr->e_phnum == 0 || ehdr->e_phentsize != sizeof(elf64_phdr_t)) {
    elf_debug("elf: bad phdr\r\n");
    return -1;
  }

  if (ehdr->e_phoff + (uint64_t)ehdr->e_phnum * ehdr->e_phentsize > size) {
    elf_debug("elf: phdr out of range\r\n");
    return -1;
  }

  const elf64_phdr_t *phdrs = (const elf64_phdr_t *)((const uint8_t *)buf + ehdr->e_phoff);

  uint64_t max_vaddr = 0;
  for (uint16_t i = 0; i < ehdr->e_phnum; ++i) {
    if (phdrs[i].p_type != PT_LOAD) continue;
    uint64_t seg_end = phdrs[i].p_vaddr + phdrs[i].p_memsz;
    if (seg_end > max_vaddr) max_vaddr = seg_end;
  }

  for (uint16_t i = 0; i < ehdr->e_phnum; ++i) {
    if (phdrs[i].p_type != PT_LOAD) continue;

    uint64_t seg_vaddr = phdrs[i].p_vaddr;
    uint64_t seg_filesz = phdrs[i].p_filesz;
    uint64_t seg_memsz = phdrs[i].p_memsz;

    if (seg_memsz == 0) continue;

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

    uint64_t pte_flags = BRIGHTS_PTE_PRESENT | BRIGHTS_PTE_USER;
    if (phdrs[i].p_flags & PF_W) {
      pte_flags |= BRIGHTS_PTE_WRITABLE;
    }

    for (uint64_t pg = 0; pg < num_pages; ++pg) {
      uint64_t vaddr = page_start + pg * 4096;
      void *phys_page = brights_pmem_alloc_page();
      if (!phys_page) {
        elf_debug("elf: pmem alloc fail\r\n");
        return -1;
      }

      kutil_memset(phys_page, 0, 4096);

      uint64_t page_copy_len = 0;

      if (seg_filesz > 0) {
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

      if (brights_paging_map((uint32_t)vaddr, (uint32_t)(uintptr_t)phys_page, (uint32_t)pte_flags) != 0) {
        elf_debug("elf: paging map fail\r\n");
        return -1;
      }
    }
  }

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
  phdr_user_vaddr = align_up_4k(phdr_user_vaddr);
  kutil_memcpy(phdr_phys, phdrs, ehdr->e_phnum * sizeof(elf64_phdr_t));
  if (brights_paging_map((uint32_t)phdr_user_vaddr, (uint32_t)(uintptr_t)phdr_phys,
                          BRIGHTS_PTE_PRESENT | BRIGHTS_PTE_USER) != 0) {
    elf_debug("elf: phdr map fail\r\n");
    return -1;
  }

  uint64_t stack_base = USER_STACK_TOP_64 - (uint64_t)USER_STACK_PAGES * 4096;
  stack_base = align_down_4k(stack_base);

  for (int i = 0; i < USER_STACK_PAGES; ++i) {
    void *sp = brights_pmem_alloc_page();
    if (!sp) {
      elf_debug("elf: stack alloc fail\r\n");
      return -1;
    }
    kutil_memset(sp, 0, 4096);
    uint64_t sv = stack_base + (uint64_t)i * 4096;
    if (brights_paging_map((uint32_t)sv, (uint32_t)(uintptr_t)sp,
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

/* ---- ELF32 loader (i386) ---- */
static int elf32_load(const void *buf, uint64_t size, elf64_load_info_t *info_out)
{
  if (!buf || size < sizeof(elf32_ehdr_t) || !info_out) {
    elf_debug("elf32: bad args\r\n");
    return -1;
  }

  const elf32_ehdr_t *ehdr = (const elf32_ehdr_t *)buf;

  if (ehdr->e_ident[0] != 0x7F || ehdr->e_ident[1] != 'E' ||
      ehdr->e_ident[2] != 'L' || ehdr->e_ident[3] != 'F') {
    elf_debug("elf32: bad magic\r\n");
    return -1;
  }

  if (ehdr->e_ident[4] != ELF_CLASS_32 || ehdr->e_ident[5] != ELF_DATA_LE ||
      ehdr->e_machine != ELF_MACH_I386) {
    elf_debug("elf32: wrong arch\r\n");
    return -1;
  }

  if (ehdr->e_type != ELF_TYPE_EXEC && ehdr->e_type != ELF_TYPE_DYN) {
    elf_debug("elf32: not executable\r\n");
    return -1;
  }

  if (ehdr->e_entry == 0) {
    elf_debug("elf32: no entry\r\n");
    return -1;
  }

  if (ehdr->e_phoff == 0 || ehdr->e_phnum == 0 || ehdr->e_phentsize != sizeof(elf32_phdr_t)) {
    elf_debug("elf32: bad phdr\r\n");
    return -1;
  }

  if (ehdr->e_phoff + (uint32_t)ehdr->e_phnum * ehdr->e_phentsize > (uint32_t)size) {
    elf_debug("elf32: phdr out of range\r\n");
    return -1;
  }

  const elf32_phdr_t *phdrs = (const elf32_phdr_t *)((const uint8_t *)buf + ehdr->e_phoff);

  uint32_t max_vaddr = 0;
  for (uint16_t i = 0; i < ehdr->e_phnum; ++i) {
    if (phdrs[i].p_type != PT_LOAD) continue;
    uint32_t seg_end = phdrs[i].p_vaddr + phdrs[i].p_memsz;
    if (seg_end > max_vaddr) max_vaddr = seg_end;
  }

  for (uint16_t i = 0; i < ehdr->e_phnum; ++i) {
    if (phdrs[i].p_type != PT_LOAD) continue;

    uint32_t seg_vaddr = phdrs[i].p_vaddr;
    uint32_t seg_filesz = phdrs[i].p_filesz;
    uint32_t seg_memsz = phdrs[i].p_memsz;

    if (seg_memsz == 0) continue;

    if (phdrs[i].p_offset + seg_filesz > (uint32_t)size) {
      elf_debug("elf32: segment out of range\r\n");
      return -1;
    }

    uint32_t page_start = (uint32_t)align_down_4k(seg_vaddr);
    uint32_t page_end = (uint32_t)align_up_4k(seg_vaddr + seg_memsz);
    uint32_t num_pages = (page_end - page_start) / 4096;

    uint32_t pte_flags = BRIGHTS_PTE_PRESENT | BRIGHTS_PTE_USER;
    if (phdrs[i].p_flags & PF_W) {
      pte_flags |= BRIGHTS_PTE_WRITABLE;
    }

    for (uint32_t pg = 0; pg < num_pages; ++pg) {
      uint32_t vaddr = page_start + pg * 4096;
      void *phys_page = brights_pmem_alloc_page();
      if (!phys_page) {
        elf_debug("elf32: pmem alloc fail\r\n");
        return -1;
      }

      kutil_memset(phys_page, 0, 4096);

      uint32_t page_copy_len = 0;

      if (seg_filesz > 0) {
        uint32_t copy_start_vaddr = (seg_vaddr > vaddr) ? seg_vaddr : vaddr;
        uint32_t copy_end_vaddr = vaddr + 4096;
        uint32_t seg_data_end = seg_vaddr + seg_filesz;
        if (seg_data_end < copy_end_vaddr) copy_end_vaddr = seg_data_end;

        if (copy_start_vaddr < copy_end_vaddr) {
          page_copy_len = copy_end_vaddr - copy_start_vaddr;
          uint32_t src_off = phdrs[i].p_offset + (copy_start_vaddr - seg_vaddr);
          uint32_t dst_off = copy_start_vaddr - vaddr;
          kutil_memcpy((uint8_t *)phys_page + dst_off,
                       (const uint8_t *)buf + src_off, page_copy_len);
        }
      }

      if (brights_paging_map(vaddr, (uint32_t)(uintptr_t)phys_page, pte_flags) != 0) {
        elf_debug("elf32: paging map fail\r\n");
        return -1;
      }
    }
  }

  uint32_t phdr_pages = (uint32_t)align_up_4k(ehdr->e_phnum * sizeof(elf32_phdr_t)) / 4096;
  if (phdr_pages == 0) phdr_pages = 1;
  void *phdr_phys = brights_pmem_alloc_page();
  if (!phdr_phys) {
    elf_debug("elf32: phdr alloc fail\r\n");
    return -1;
  }
  kutil_memset(phdr_phys, 0, 4096);
  uint32_t phdr_user_vaddr = (uint32_t)align_up_4k(max_vaddr);
  if (phdr_user_vaddr < max_vaddr) phdr_user_vaddr = max_vaddr;
  phdr_user_vaddr = (uint32_t)align_up_4k(phdr_user_vaddr);
  kutil_memcpy(phdr_phys, phdrs, ehdr->e_phnum * sizeof(elf32_phdr_t));
  if (brights_paging_map(phdr_user_vaddr, (uint32_t)(uintptr_t)phdr_phys,
                          BRIGHTS_PTE_PRESENT | BRIGHTS_PTE_USER) != 0) {
    elf_debug("elf32: phdr map fail\r\n");
    return -1;
  }

  uint32_t stack_base = USER_STACK_TOP_32 - (uint32_t)USER_STACK_PAGES * 4096;
  stack_base = (uint32_t)align_down_4k(stack_base);

  for (int i = 0; i < USER_STACK_PAGES; ++i) {
    void *sp = brights_pmem_alloc_page();
    if (!sp) {
      elf_debug("elf32: stack alloc fail\r\n");
      return -1;
    }
    kutil_memset(sp, 0, 4096);
    uint32_t sv = stack_base + (uint32_t)i * 4096;
    if (brights_paging_map(sv, (uint32_t)(uintptr_t)sp,
                            BRIGHTS_PTE_PRESENT | BRIGHTS_PTE_WRITABLE | BRIGHTS_PTE_USER) != 0) {
      elf_debug("elf32: stack map fail\r\n");
      return -1;
    }
  }

  info_out->entry = ehdr->e_entry;
  info_out->stack_top = stack_base + (uint32_t)USER_STACK_PAGES * 4096;
  info_out->phdr = phdr_user_vaddr;
  info_out->phnum = ehdr->e_phnum;
  info_out->phentsize = ehdr->e_phentsize;

  elf_debug("elf32: loaded entry=");
  elf_debug_hex64(info_out->entry);
  elf_debug(" stack=");
  elf_debug_hex64(info_out->stack_top);
  elf_debug("\r\n");

  return 0;
}

/* Main ELF loader: dispatches to 32-bit or 64-bit loader based on ELF class */
int brights_elf_load(const void *buf, uint64_t size, elf64_load_info_t *info_out)
{
  if (!buf || size < 16 || !info_out) {
    elf_debug("elf: bad args\r\n");
    return -1;
  }

  const uint8_t *ident = (const uint8_t *)buf;

  if (ident[0] != 0x7F || ident[1] != 'E' || ident[2] != 'L' || ident[3] != 'F') {
    elf_debug("elf: bad magic\r\n");
    return -1;
  }

  if (ident[4] == ELF_CLASS_32) {
    return elf32_load(buf, size, info_out);
  } else if (ident[4] == ELF_CLASS_64) {
    return elf64_load(buf, size, info_out);
  }

  elf_debug("elf: unknown class\r\n");
  return -1;
}
