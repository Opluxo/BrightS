#ifndef BRIGHTS_ELF_H
#define BRIGHTS_ELF_H

#include <stdint.h>

/* ELF64 types */
#define ELF_MAGIC 0x464C457Fu  /* "\x7fELF" */

#define ELF_CLASS_32 1
#define ELF_CLASS_64 2
#define ELF_DATA_LE 1
#define ELF_MACH_I386 0x03
#define ELF_MACH_X86_64 0x3E
#define ELF_TYPE_EXEC 2
#define ELF_TYPE_DYN 3

#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3

#define PF_X 0x1
#define PF_W 0x2
#define PF_R 0x4

/* ELF64 header and program header */
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

/* ELF32 header and program header */
typedef struct {
  uint8_t  e_ident[16];
  uint16_t e_type;
  uint16_t e_machine;
  uint32_t e_version;
  uint32_t e_entry;
  uint32_t e_phoff;
  uint32_t e_shoff;
  uint32_t e_flags;
  uint16_t e_ehsize;
  uint16_t e_phentsize;
  uint16_t e_phnum;
  uint16_t e_shentsize;
  uint16_t e_shnum;
  uint16_t e_shstrndx;
} __attribute__((packed)) elf32_ehdr_t;

typedef struct {
  uint32_t p_type;
  uint32_t p_offset;
  uint32_t p_vaddr;
  uint32_t p_paddr;
  uint32_t p_filesz;
  uint32_t p_memsz;
  uint32_t p_flags;
  uint32_t p_align;
} __attribute__((packed)) elf32_phdr_t;

/* Common load info (fields hold 32-bit values for ELF32, 64-bit for ELF64) */
typedef struct {
  uint64_t entry;       /* Entry point address */
  uint64_t stack_top;   /* Top of allocated user stack */
  uint64_t phdr;        /* Address of program headers in user memory */
  uint32_t phnum;       /* Number of program headers */
  uint32_t phentsize;   /* Size of each program header */
} elf64_load_info_t;

/*
 * Load an ELF executable from a memory buffer.
 * Supports both ELF32 and ELF64.
 * Maps PT_LOAD segments into user-space pages and sets up a user stack.
 */
int brights_elf_load(const void *buf, uint64_t size, elf64_load_info_t *info_out);

#endif
