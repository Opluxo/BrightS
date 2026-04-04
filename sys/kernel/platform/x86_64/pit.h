#ifndef BRIGHTS_PIT_H
#define BRIGHTS_PIT_H

#include <stdint.h>

/* Initialize PIT channel 0 for periodic interrupts at given frequency */
void brights_pit_init(uint32_t frequency_hz);

#endif
