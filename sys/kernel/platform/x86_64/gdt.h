#ifndef BRIGHTS_X86_64_GDT_H
#define BRIGHTS_X86_64_GDT_H

#include <stdint.h>

#define BRIGHTS_KERNEL_CS 0x08u
#define BRIGHTS_KERNEL_DS 0x10u
#define BRIGHTS_USER_DS   0x20u
#define BRIGHTS_USER_CS   0x28u
#define BRIGHTS_TSS_SEL   0x28u

void brights_gdt_init(void);

#endif
