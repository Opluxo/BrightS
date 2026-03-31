#ifndef BRIGHTS_SLEEP_H
#define BRIGHTS_SLEEP_H

#include <stdint.h>

// Sleep for specified CPU cycles
void brights_sleep_cycles(uint64_t cycles);

// Sleep for specified microseconds (approximate)
void brights_sleep_us(uint64_t us);

// Sleep for specified milliseconds
void brights_sleep_ms(uint64_t ms);

// Halt CPU until next interrupt (power saving)
void brights_halt(void);

#endif
