#ifndef BRIGHTS_SYSENT_H
#define BRIGHTS_SYSENT_H
#include <stdint.h>
typedef struct { uint64_t nargs; int64_t (*handler)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t); } brights_sysent_t;
extern brights_sysent_t brights_sysent_table[];
extern const uint64_t brights_sysent_count;
int64_t brights_sys_dispatch(uint64_t nr, uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5);
#endif
