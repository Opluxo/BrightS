#ifndef BRIGHTS_CLOCK_H
#define BRIGHTS_CLOCK_H

#include <stdint.h>

void brights_clock_init(void);
void brights_clock_tick(void);
uint64_t brights_clock_now_ticks(void);
void brights_clock_advance(uint64_t ticks);

#endif
