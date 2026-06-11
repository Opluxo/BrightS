#ifndef BRIGHTS_PIC_H
#define BRIGHTS_PIC_H

#include <stdint.h>

/* Remap PIC so IRQ 0-7 → INT 32-39, IRQ 8-15 → INT 40-47 */
void brights_pic_remap(void);

/* Send End-Of-Interrupt to PIC */
void brights_pic_eoi(uint8_t irq);

/* Mask/unmask an IRQ line */
void brights_pic_mask(uint8_t irq);
void brights_pic_unmask(uint8_t irq);

#endif
