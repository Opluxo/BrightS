#ifndef BRIGHTS_I386_PIC_H
#define BRIGHTS_I386_PIC_H

#include <stdint.h>

void brights_pic_remap(void);
void brights_pic_eoi(uint8_t irq);
void brights_pic_mask(uint8_t irq);
void brights_pic_unmask(uint8_t irq);

#endif
