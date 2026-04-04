#ifndef BRIGHTS_CPU_LOCAL_H
#define BRIGHTS_CPU_LOCAL_H
#include <stdint.h>
void brights_cpu_local_init(uint64_t kernel_rsp);
void brights_cpu_local_init_ap(uint32_t apic_id, uint64_t kernel_rsp);
uint32_t brights_cpu_local_id(void);
uint32_t brights_cpu_local_current_pid(void);
void brights_cpu_local_set_pid(uint32_t pid);
#endif
