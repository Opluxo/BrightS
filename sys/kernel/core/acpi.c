#include "acpi.h"
#include "../platform/x86_64/io.h"
#include <stdint.h>

#define ACPI_SIG_FADT 0x50434146u
#define ACPI_RESET_REG_SUPPORTED (1u << 10)

typedef struct {
  char signature[8];
  uint8_t checksum;
  char oem_id[6];
  uint8_t revision;
  uint32_t rsdt_addr;
  uint32_t length;
  uint64_t xsdt_addr;
  uint8_t ext_checksum;
  uint8_t reserved[3];
} __attribute__((packed)) acpi_rsdp_t;

typedef struct {
  uint32_t signature;
  uint32_t length;
  uint8_t revision;
  uint8_t checksum;
  char oem_id[6];
  char oem_table_id[8];
  uint32_t oem_revision;
  uint32_t creator_id;
  uint32_t creator_revision;
} __attribute__((packed)) acpi_sdt_t;

typedef struct {
  uint8_t space_id;
  uint8_t bit_width;
  uint8_t bit_offset;
  uint8_t access_size;
  uint64_t address;
} __attribute__((packed)) acpi_gas_t;

typedef struct {
  acpi_sdt_t hdr;
  uint32_t firmware_ctrl;
  uint32_t dsdt;
  uint8_t reserved0;
  uint8_t preferred_pm_profile;
  uint16_t sci_int;
  uint32_t smi_cmd;
  uint8_t acpi_enable;
  uint8_t acpi_disable;
  uint8_t s4bios_req;
  uint8_t pstate_cnt;
  uint32_t pm1a_evt_blk;
  uint32_t pm1b_evt_blk;
  uint32_t pm1a_cnt_blk;
  uint32_t pm1b_cnt_blk;
  uint32_t pm2_cnt_blk;
  uint32_t pm_tmr_blk;
  uint32_t gpe0_blk;
  uint32_t gpe1_blk;
  uint8_t pm1_evt_len;
  uint8_t pm1_cnt_len;
  uint8_t pm2_cnt_len;
  uint8_t pm_tmr_len;
  uint8_t gpe0_blk_len;
  uint8_t gpe1_blk_len;
  uint8_t gpe1_base;
  uint8_t cst_cnt;
  uint16_t p_lvl2_lat;
  uint16_t p_lvl3_lat;
  uint16_t flush_size;
  uint16_t flush_stride;
  uint8_t duty_offset;
  uint8_t duty_width;
  uint8_t day_alrm;
  uint8_t mon_alrm;
  uint8_t century;
  uint16_t iapc_boot_arch;
  uint8_t reserved1;
  uint32_t flags;
  acpi_gas_t reset_reg;
  uint8_t reset_value;
  uint16_t arm_boot_arch;
  uint8_t fadt_minor_version;
  uint64_t x_firmware_ctrl;
  uint64_t x_dsdt;
  acpi_gas_t x_pm1a_evt_blk;
  acpi_gas_t x_pm1b_evt_blk;
  acpi_gas_t x_pm1a_cnt_blk;
  acpi_gas_t x_pm1b_cnt_blk;
  acpi_gas_t x_pm2_cnt_blk;
  acpi_gas_t x_pm_tmr_blk;
  acpi_gas_t x_gpe0_blk;
  acpi_gas_t x_gpe1_blk;
} __attribute__((packed)) acpi_fadt_t;

static uint64_t acpi_rsdp_addr;
static brights_acpi_info_t acpi_info_state;

static int checksum_ok(const void *ptr, uint32_t len)
{
  const uint8_t *p = (const uint8_t *)ptr;
  uint8_t sum = 0;
  for (uint32_t i = 0; i < len; ++i) {
    sum = (uint8_t)(sum + p[i]);
  }
  return sum == 0;
}

static uint32_t sig32(const char *s)
{
  return ((uint32_t)(uint8_t)s[0]) |
         ((uint32_t)(uint8_t)s[1] << 8) |
         ((uint32_t)(uint8_t)s[2] << 16) |
         ((uint32_t)(uint8_t)s[3] << 24);
}

static void copy_trim(char *dst, uint32_t cap, const char *src, uint32_t len)
{
  uint32_t n = 0;
  if (!dst || cap == 0) {
    return;
  }
  while (n + 1 < cap && n < len && src[n]) {
    dst[n] = src[n];
    ++n;
  }
  while (n > 0 && dst[n - 1] == ' ') {
    --n;
  }
  dst[n] = 0;
}

void brights_acpi_bootstrap(uint64_t rsdp_addr)
{
  acpi_rsdp_addr = rsdp_addr;
}

int brights_acpi_init(void)
{
  acpi_rsdp_t *rsdp = (acpi_rsdp_t *)(uintptr_t)acpi_rsdp_addr;
  if (!rsdp || acpi_rsdp_addr == 0) {
    return -1;
  }
  if (!checksum_ok(rsdp, 20)) {
    return -1;
  }
  if (rsdp->revision >= 2 && (!rsdp->length || !checksum_ok(rsdp, rsdp->length))) {
    return -1;
  }

  uint64_t xsdt_addr = (rsdp->revision >= 2) ? rsdp->xsdt_addr : 0;
  acpi_sdt_t *xsdt = (acpi_sdt_t *)(uintptr_t)xsdt_addr;
  if (!xsdt || xsdt_addr == 0 || xsdt->signature != sig32("XSDT")) {
    return -1;
  }
  if (!checksum_ok(xsdt, xsdt->length)) {
    return -1;
  }

  acpi_info_state.revision = rsdp->revision;
  acpi_info_state.has_xsdt = 1;
  copy_trim(acpi_info_state.oem_id, sizeof(acpi_info_state.oem_id), xsdt->oem_id, 6);
  copy_trim(acpi_info_state.oem_table_id, sizeof(acpi_info_state.oem_table_id), xsdt->oem_table_id, 8);

  uint32_t entries = (xsdt->length - sizeof(acpi_sdt_t)) / 8u;
  uint64_t *table_addrs = (uint64_t *)((uint8_t *)xsdt + sizeof(acpi_sdt_t));
  for (uint32_t i = 0; i < entries; ++i) {
    acpi_sdt_t *hdr = (acpi_sdt_t *)(uintptr_t)table_addrs[i];
    if (!hdr || hdr->signature != ACPI_SIG_FADT) {
      continue;
    }
    if (!checksum_ok(hdr, hdr->length)) {
      continue;
    }

    acpi_fadt_t *fadt = (acpi_fadt_t *)hdr;
    acpi_info_state.has_fadt = 1;
    acpi_info_state.pm_profile = fadt->preferred_pm_profile;
    acpi_info_state.sci_int = fadt->sci_int;
    acpi_info_state.flags = fadt->flags;
    if (fadt->x_pm_tmr_blk.address != 0) {
      acpi_info_state.pm_tmr_blk = fadt->x_pm_tmr_blk.address;
    } else {
      acpi_info_state.pm_tmr_blk = fadt->pm_tmr_blk;
    }
    if ((fadt->flags & ACPI_RESET_REG_SUPPORTED) && fadt->reset_reg.address != 0) {
      acpi_info_state.has_reset_reg = 1;
      acpi_info_state.reset_reg_addr = fadt->reset_reg.address;
      acpi_info_state.reset_reg_space = fadt->reset_reg.space_id;
      acpi_info_state.reset_value = fadt->reset_value;
    }
    break;
  }

  acpi_info_state.ready = 1;
  return 0;
}

const brights_acpi_info_t *brights_acpi_info(void)
{
  return &acpi_info_state;
}

int brights_acpi_reboot(void)
{
  if (!acpi_info_state.ready || !acpi_info_state.has_reset_reg) {
    return -1;
  }
  if (acpi_info_state.reset_reg_space != 1) {
    return -1;
  }

  uint64_t addr = acpi_info_state.reset_reg_addr;
  if (addr > 0xFFFFu) {
    return -1;
  }
  outb((uint16_t)addr, acpi_info_state.reset_value);
  return 0;
}
