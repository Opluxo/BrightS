#ifndef BRIGHTS_CLOCK_H
#define BRIGHTS_CLOCK_H

#include <stdint.h>

// Initialize clock subsystem
void brights_clock_init(void);

// Advance clock by one tick (called from timer interrupt)
void brights_clock_tick(void);

// Get current tick count
uint64_t brights_clock_now_ticks(void);

// Advance clock by specified ticks
void brights_clock_advance(uint64_t ticks);

// Get current time in nanoseconds (using TSC if available)
uint64_t brights_clock_ns(void);

// Get current time in microseconds
uint64_t brights_clock_us(void);

// Get current time in milliseconds
uint64_t brights_clock_ms(void);

// Calibrate TSC frequency (call once at boot)
void brights_clock_calibrate(void);

// Get TSC frequency in Hz
uint64_t brights_clock_tsc_freq(void);

#endif
