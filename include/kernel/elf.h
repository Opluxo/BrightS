#ifndef BRIGHTS_ELF_H
#define BRIGHTS_ELF_H

#include <stdint.h>

/* ELF64 types */
#define ELF_MAGIC 0x464C457Fu  /* "\x7fELF" */

#define ELF_CLASS_64 2
#define ELF_DATA_LE 1
#define ELF_MACH_X86_64 0x3E
#define ELF_TYPE_EXEC 2
#define ELF_TYPE_DYN 3

#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3

#define PF_X 0x1
#define PF_W 0x2
#define PF_R 0x4

typedef struct {
  uint8_t  e_ident[16];
  uint16_t e_type;
  uint16_t e_machine;
  uint32_t e_version;
  uint64_t e_entry;
  uint64_t e_phoff;
  uint64_t e_shoff;
  uint32_t e_flags;
  uint16_t e_ehsize;
  uint16_t e_phentsize;
  uint16_t e_phnum;
  uint16_t e_shentsize;
  uint16_t e_shnum;
  uint16_t e_shstrndx;
} __attribute__((packed)) elf64_ehdr_t;

typedef struct {
  uint32_t p_type;
  uint32_t p_flags;
  uint64_t p_offset;
  uint64_t p_vaddr;
  uint64_t p_paddr;
  uint64_t p_filesz;
  uint64_t p_memsz;
  uint64_t p_align;
} __attribute__((packed)) elf64_phdr_t;

typedef struct {
  uint64_t entry;       /* Entry point address */
  uint64_t stack_top;   /* Top of allocated user stack */
  uint64_t phdr;        /* Address of program headers in user memory */
  uint32_t phnum;       /* Number of program headers */
  uint32_t phentsize;   /* Size of each program header */
} elf64_load_info_t;

/*
 * Load an ELF64 executable from a memory buffer.
 * Maps PT_LOAD segments into user-space pages and sets up a user stack.
 *
 * buf: pointer to ELF file data in memory
 * size: size of the ELF file data
 * info_out: receives load information (entry point, stack top, etc.)
 *
 * Returns 0 on success, -1 on failure.
 */
int brights_elf_load(const void *buf, uint64_t size, elf64_load_info_t *info_out);

#endif
