#ifndef BRIGHTS_APIC_H
#define BRIGHTS_APIC_H

#include <stdint.h>

/* Initialize Local APIC for the BSP (bootstrap processor) */
int brights_apic_init(void);

/* Check if APIC is available and enabled */
int brights_apic_available(void);

/* Get Local APIC ID of current CPU */
uint32_t brights_apic_id(void);

/* Send End-Of-Interrupt to Local APIC */
void brights_apic_eoi(void);

/* Configure Local APIC timer for periodic interrupts */
void brights_apic_timer_init(uint32_t frequency_hz);

/* Calibrate Local APIC timer using PIT */
void brights_apic_timer_calibrate(void);

/* Get APIC timer frequency */
uint32_t brights_apic_timer_freq(void);

/* Send IPI to another CPU */
void brights_apic_send_ipi(uint32_t apic_id, uint8_t vector);

/* Read APIC register */
uint32_t brights_apic_read(uint32_t reg);

/* Write APIC register */
void brights_apic_write(uint32_t reg, uint32_t val);

#endif
