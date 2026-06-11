#ifndef BRIGHTS_IOAPIC_H
#define BRIGHTS_IOAPIC_H

#include <stdint.h>

/* Initialize IOAPIC at given MMIO address */
int brights_ioapic_init(uint64_t mmio_addr);

/* Set an IOAPIC redirect entry */
/* gsi: Global System Interrupt number */
/* vector: interrupt vector (e.g., 0x20 for IRQ0) */
/* delivery: 0=fixed, 1=lowest priority, 2=SMI, 4=NMI, 5=INIT */
/* dest_apic_id: destination Local APIC ID */
void brights_ioapic_set_redirect(uint32_t gsi, uint8_t vector,
                                  uint8_t delivery, uint32_t dest_apic_id);

/* Mask/unmask a GSI */
void brights_ioapic_mask(uint32_t gsi);
void brights_ioapic_unmask(uint32_t gsi);

/* Get number of GSI entries */
uint32_t brights_ioapic_gsi_count(void);

#endif
