#include "uefi.h"
#include "uefi_memmap.h"
#include "../../core/printf.h"
#include "../../core/acpi.h"
#include "../../core/vm.h"
#include "../../dev/serial.h"
#include "idt.h"
#include "syscall_abi.h"
#include "../../fs/vfs.h"
#include "../../core/kernel_main.h"

static void uefi_puts(brights_console_t *con, const uint16_t *msg)
{
  EFI_SYSTEM_TABLE *st = (EFI_SYSTEM_TABLE *)con->ctx;
  st->ConOut->OutputString(st->ConOut, msg);
}

static void uefi_print_hex(brights_console_t *con, uint64_t v)
{
  static const uint16_t hex[] = u"0123456789ABCDEF";
  uint16_t buf[2 + 16 + 1];
  buf[0] = '0';
  buf[1] = 'x';
  for (int i = 0; i < 16; ++i) {
    buf[2 + i] = hex[(v >> ((15 - i) * 4)) & 0xF];
  }
  buf[18] = 0;
  brights_print(con, buf);
}

static void uefi_print_str(brights_console_t *con, const uint16_t *msg)
{
  brights_print(con, msg);
}

static void uefi_print_nl(brights_console_t *con)
{
  brights_print(con, u"\r\n");
}

static void uefi_dump_memmap(brights_console_t *con, EFI_SYSTEM_TABLE *st)
{
  uint64_t map_size = 0;
  uint64_t map_key = 0;
  uint64_t desc_size = 0;
  uint32_t desc_ver = 0;
  EFI_STATUS status;

  status = st->BootServices->GetMemoryMap(&map_size, 0, &map_key, &desc_size, &desc_ver);
  if (status == 0 && map_size == 0) {
    uefi_print_str(con, u"memmap: empty\r\n");
    return;
  }

  map_size += desc_size * 8;
  EFI_PHYSICAL_ADDRESS map_addr = 0;
  status = st->BootServices->AllocatePages(0, 0, (map_size + 4095) / 4096, &map_addr);
  if (status != 0) {
    uefi_print_str(con, u"memmap: alloc fail\r\n");
    return;
  }

  EFI_MEMORY_DESCRIPTOR *map = (EFI_MEMORY_DESCRIPTOR *)(uintptr_t)map_addr;
  status = st->BootServices->GetMemoryMap(&map_size, map, &map_key, &desc_size, &desc_ver);
  if (status != 0) {
    uefi_print_str(con, u"memmap: get fail\r\n");
    return;
  }

  uint8_t *iter = (uint8_t *)map;
  uint64_t entries = map_size / desc_size;
  uefi_print_str(con, u"memmap entries: ");
  uefi_print_hex(con, entries);
  uefi_print_nl(con);

  for (uint64_t i = 0; i < entries; ++i) {
    EFI_MEMORY_DESCRIPTOR *d = (EFI_MEMORY_DESCRIPTOR *)(iter + i * desc_size);
    uefi_print_str(con, u"  type=");
    uefi_print_hex(con, d->Type);
    uefi_print_str(con, u" base=");
    uefi_print_hex(con, d->PhysicalStart);
    uefi_print_str(con, u" pages=");
    uefi_print_hex(con, d->NumberOfPages);
    uefi_print_nl(con);
  }
}

static void uefi_dump_meminfo(brights_console_t *con, const brights_uefi_memmap_info_t *info)
{
  uefi_print_str(con, u"memmap regions=");
  uefi_print_hex(con, info->region_count);
  uefi_print_str(con, u" total=");
  uefi_print_hex(con, info->total_bytes);
  uefi_print_nl(con);
}

static int guid_eq(const EFI_GUID *a, const EFI_GUID *b)
{
  if (a->Data1 != b->Data1 || a->Data2 != b->Data2 || a->Data3 != b->Data3) {
    return 0;
  }
  for (int i = 0; i < 8; ++i) {
    if (a->Data4[i] != b->Data4[i]) {
      return 0;
    }
  }
  return 1;
}

static uint64_t uefi_find_rsdp(EFI_SYSTEM_TABLE *st)
{
  static const EFI_GUID acpi20 = {0x8868e871u, 0xe4f1u, 0x11d3u, {0xbcu, 0x22u, 0x00u, 0x80u, 0xc7u, 0x3cu, 0x88u, 0x81u}};
  static const EFI_GUID acpi10 = {0xeb9d2d30u, 0x2d88u, 0x11d3u, {0x9au, 0x16u, 0x00u, 0x90u, 0x27u, 0x3fu, 0xc1u, 0x4du}};
  EFI_CONFIGURATION_TABLE *cfg = (EFI_CONFIGURATION_TABLE *)st->ConfigurationTable;

  for (uint64_t i = 0; i < st->NumberOfTableEntries; ++i) {
    if (guid_eq(&cfg[i].VendorGuid, &acpi20)) {
      return (uint64_t)(uintptr_t)cfg[i].VendorTable;
    }
  }
  for (uint64_t i = 0; i < st->NumberOfTableEntries; ++i) {
    if (guid_eq(&cfg[i].VendorGuid, &acpi10)) {
      return (uint64_t)(uintptr_t)cfg[i].VendorTable;
    }
  }
  return 0;
}

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
  (void)ImageHandle;
  brights_console_t con;
  brights_console_t serial_con;
  brights_console_init(&con, SystemTable, uefi_puts);
  brights_serial_console_init(&serial_con, BRIGHTS_COM1_PORT);

  uefi_print_str(&con, u"BrightS kernel: UEFI entry ok\r\n");
  uefi_print_str(&serial_con, u"BrightS kernel: serial ok\r\n");

  uint64_t rsdp_addr = uefi_find_rsdp(SystemTable);
  if (rsdp_addr != 0) {
    brights_acpi_bootstrap(rsdp_addr);
    uefi_print_str(&serial_con, u"acpi: rsdp found\r\n");
  } else {
    uefi_print_str(&serial_con, u"acpi: rsdp missing\r\n");
  }

  uefi_dump_memmap(&serial_con, SystemTable);

  brights_uefi_memmap_info_t meminfo;
  int mem_ok = brights_uefi_parse_memmap(SystemTable, &meminfo);
  if (mem_ok == 0) {
    uefi_dump_meminfo(&serial_con, &meminfo);
    brights_vm_bootstrap(meminfo.regions, meminfo.region_count);
  } else {
    uefi_print_str(&serial_con, u"memmap parse failed\r\n");
  }

  EFI_STATUS status = 1;
  if (mem_ok == 0) {
    status = SystemTable->BootServices->ExitBootServices(ImageHandle, meminfo.map_key);
    if (status != 0) {
      // Retry once with a fresh memory map key.
      mem_ok = brights_uefi_parse_memmap(SystemTable, &meminfo);
      if (mem_ok == 0) {
        status = SystemTable->BootServices->ExitBootServices(ImageHandle, meminfo.map_key);
      }
    }
  }

  if (status != 0) {
    uefi_print_str(&serial_con, u"ExitBootServices failed\r\n");
  } else {
    uefi_print_str(&serial_con, u"ExitBootServices ok\r\n");
  }

  brights_vm_init();
  uefi_print_str(&serial_con, u"BrightS kernel: vm ok\r\n");
  brights_idt_init();
  uefi_print_str(&serial_con, u"BrightS kernel: idt ok\r\n");
  brights_syscall_abi_init();
  brights_vfs_init();
  uefi_print_str(&serial_con, u"BrightS kernel: syscall/vfs ok\r\n");
  brights_kernel_main();

  for (;;) {
    __asm__ __volatile__("hlt");
  }
  return 0;
}
