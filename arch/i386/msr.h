#ifndef BRIGHTS_I386_MSR_H
#define BRIGHTS_I386_MSR_H

#include <stdint.h>

static inline uint64_t rdmsr(uint32_t msr)
{
    uint32_t lo, hi;
    __asm__ __volatile__("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((uint64_t)hi << 32) | lo;
}

static inline void wrmsr(uint32_t msr, uint64_t val)
{
    uint32_t lo = val & 0xFFFFFFFF;
    uint32_t hi = val >> 32;
    __asm__ __volatile__("wrmsr" :: "a"(lo), "d"(hi), "c"(msr));
}

#endif
