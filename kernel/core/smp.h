#ifndef BRIGHTS_SMP_H
#define BRIGHTS_SMP_H

#include <stdint.h>

/* Initialize SMP: start all APs */
int brights_smp_init(void);

/* Get number of detected CPU cores */
uint32_t brights_smp_core_count(void);

/* Get APIC ID of a core index */
uint32_t brights_smp_apic_id(uint32_t core_idx);

/* AP entry point (called by each AP during boot) */
void brights_smp_ap_entry(uint32_t apic_id);

/* Request scheduling on a specific AP */
void brights_smp_schedule_ap(uint32_t apic_id);

/* Broadcast scheduler IPI to all APs */
void brights_smp_broadcast_sched(void);

#endif
