#ifndef BRIGHTS_ACPI_H
#define BRIGHTS_ACPI_H

#include <stdint.h>

#define BRIGHTS_MAX_IOAPIC   4
#define BRIGHTS_MAX_LAPIC_OVERRIDE 4
#define BRIGHTS_MAX_LAPIC_NMI 8

/* IOAPIC entry from MADT */
typedef struct {
  uint8_t id;
  uint64_t mmio_addr;
  uint32_t gsi_base;
} brights_ioapic_entry_t;

/* Local APIC override (interrupt source override) */
typedef struct {
  uint8_t bus_source;
  uint8_t irq_source;
  uint32_t gsi;
  uint16_t flags;
} brights_lapic_override_t;

/* NMI source */
typedef struct {
  uint8_t lapic_id;  /* 0xFF = all */
  uint16_t flags;
  uint8_t lint;
} brights_lapic_nmi_t;

typedef struct {
  char oem_id[7];
  char oem_table_id[9];
  uint32_t revision;
  uint32_t pm_profile;
  uint32_t sci_int;
  uint32_t flags;
  uint64_t pm_tmr_blk;
  uint64_t reset_reg_addr;
  uint8_t reset_reg_space;
  uint8_t reset_value;
  int has_xsdt;
  int has_fadt;
  int has_reset_reg;
  int ready;

  /* MADT (APIC) info */
  uint64_t lapic_mmio;        /* Local APIC MMIO base address */
  uint32_t ioapic_count;
  brights_ioapic_entry_t ioapic[BRIGHTS_MAX_IOAPIC];
  uint32_t override_count;
  brights_lapic_override_t override[BRIGHTS_MAX_LAPIC_OVERRIDE];
  uint32_t nmi_count;
  brights_lapic_nmi_t nmi[BRIGHTS_MAX_LAPIC_NMI];

  /* HPET info */
  uint64_t hpet_mmio;         /* HPET MMIO base address */
  uint32_t hpet_period_fs;    /* Minimum tick period in femtoseconds */
  int has_hpet;
  int has_madt;
} brights_acpi_info_t;

void brights_acpi_bootstrap(uint64_t rsdp_addr);
int brights_acpi_init(void);
const brights_acpi_info_t *brights_acpi_info(void);
int brights_acpi_reboot(void);

#endif
