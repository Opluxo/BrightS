#ifndef BRIGHTS_I386_GDT_H
#define BRIGHTS_I386_GDT_H

#include <stdint.h>

#define BRIGHTS_KERNEL_CS 0x08
#define BRIGHTS_KERNEL_DS 0x10

#define BRIGHTS_I386_USER_DS  0x1Bu
#define BRIGHTS_I386_USER_CS  0x23u
#define BRIGHTS_I386_TSS_SEL  0x28u

void brights_gdt_init(void);
void brights_tss_init(uint32_t esp0);
uint32_t brights_tss_esp0(void);

#endif
