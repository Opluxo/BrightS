#ifndef BRIGHTS_ACPI_H
#define BRIGHTS_ACPI_H

#include <stdint.h>

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
} brights_acpi_info_t;

void brights_acpi_bootstrap(uint64_t rsdp_addr);
int brights_acpi_init(void);
const brights_acpi_info_t *brights_acpi_info(void);
int brights_acpi_reboot(void);

#endif
