#ifndef BRIGHTS_HPET_H
#define BRIGHTS_HPET_H

#include <stdint.h>

/* Initialize HPET from ACPI info */
int brights_hpet_init(uint64_t mmio_addr);

/* Read HPET main counter (nanoseconds if tick_period is known) */
uint64_t brights_hpet_read_counter(void);

/* Get HPET counter period in femtoseconds (fs) */
uint32_t brights_hpet_period_fs(void);

/* Get HPET frequency in Hz */
uint64_t brights_hpet_freq(void);

/* Sleep for given nanoseconds using HPET */
void brights_hpet_nsleep(uint64_t ns);

#endif
