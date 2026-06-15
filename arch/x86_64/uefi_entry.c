#include "uefi.h"
#include "uefi_memmap.h"
#include "io.h"
#include "paging.h"
#include "../../kernel/core/printf.h"
#include "../../kernel/core/acpi.h"
#include "../../kernel/core/vm.h"
#include "../../kernel/core/kernel_main.h"
#include "../../drivers/serial.h"
#include "idt.h"
#include "syscall_abi.h"
#include "../../kernel/core/pmem.h"
#include "../../kernel/fs/boot_fs.h"
#include <stdint.h>

/* MSVC runtime: suppress linker error for _fltused (no float used) */
int _fltused = 0;

/* Dedicated kernel stack — 256KB to avoid overflow during deep init chains.
 * Placed in .data (initialized) to avoid potential BSS issues with PE/COFF. */
#define KERNEL_STACK_SIZE (256 * 1024)
static uint8_t kernel_stack[KERNEL_STACK_SIZE] __attribute__((aligned(16)));

/* VGA framebuffer info — populated by PCI VGA scan after ExitBootServices */
typedef struct {
  uint64_t framebuffer;   /* Physical address of framebuffer */
  uint32_t width;
  uint32_t height;
  uint32_t pitch;         /* Bytes per scanline */
  uint32_t valid;         /* 1 if VGA framebuffer was found */
} vga_fb_info_t;
vga_fb_info_t vga_fb_info __attribute__((used)) = {0};

/*
 * VGA graphics mode initialization via Bochs VBE dispi interface.
 * QEMU -vga std emulates a Bochs VGA with VBE extension.
 * The dispi interface uses I/O ports 0x1CE (index) and 0x1CF (data).
 */
#define VBE_DISPI_INDEX_ID          0
#define VBE_DISPI_INDEX_XRES        1
#define VBE_DISPI_INDEX_YRES        2
#define VBE_DISPI_INDEX_BPP         3
#define VBE_DISPI_INDEX_ENABLE      4
#define VBE_DISPI_INDEX_BANK        5
#define VBE_DISPI_INDEX_LFB_ADDR_HI 6
#define VBE_DISPI_INDEX_LFB_ADDR_LO 7
#define VBE_DISPI_INDEX_VIRT_WIDTH  8
#define VBE_DISPI_INDEX_VIRT_HEIGHT 9
#define VBE_DISPI_INDEX_X_OFFSET    10
#define VBE_DISPI_INDEX_Y_OFFSET    11
#define VBE_DISPI_ID                0xB0C5
#define VBE_DISPI_ENABLED           0x01
#define VBE_DISPI_LFB_ENABLED       0x40

static inline void vbe_write(uint16_t index, uint16_t data)
{
  outw(0x1CE, index);
  outw(0x1CF, data);
}

static inline uint16_t vbe_read(uint16_t index)
{
  outw(0x1CE, index);
  return inw(0x1CF);
}

/* Returns 1 if Bochs VBE dispi interface was initialized successfully */
static int vga_init_vbe(uint32_t width, uint32_t height, uint32_t bpp)
{
  /* Check for Bochs VBE signature */
  uint16_t id = vbe_read(VBE_DISPI_INDEX_ID);
  if (id != VBE_DISPI_ID) return 0;

  /* Disable VBE before reconfiguration */
  vbe_write(VBE_DISPI_INDEX_ENABLE, 0);

  /* Set resolution and color depth */
  vbe_write(VBE_DISPI_INDEX_XRES, (uint16_t)width);
  vbe_write(VBE_DISPI_INDEX_YRES, (uint16_t)height);
  vbe_write(VBE_DISPI_INDEX_BPP, (uint16_t)bpp);

  /* Set virtual framebuffer size (same as physical, no scrolling) */
  vbe_write(VBE_DISPI_INDEX_VIRT_WIDTH, (uint16_t)width);
  vbe_write(VBE_DISPI_INDEX_VIRT_HEIGHT, (uint16_t)height);
  vbe_write(VBE_DISPI_INDEX_X_OFFSET, 0);
  vbe_write(VBE_DISPI_INDEX_Y_OFFSET, 0);

  /* Enable VBE + linear framebuffer (LFB) mode */
  vbe_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);

  /* Verify */
  uint16_t en = vbe_read(VBE_DISPI_INDEX_ENABLE);
  return (en & VBE_DISPI_ENABLED) ? 1 : 0;
}

/* PCI VGA framebuffer detection — runs after ExitBootServices */
static uint32_t vga_pci_addr = 0;
static uint32_t vga_pci_id = 0;

static void detect_pci_vga_fb(void)
{
  #define PCI_CONFIG_ADDR 0xCF8
  #define PCI_CONFIG_DATA 0xCFC

  for (uint32_t bus = 0; bus < 256; ++bus) {
    for (uint32_t dev = 0; dev < 32; ++dev) {
      for (uint32_t func = 0; func < 8; ++func) {
        uint32_t addr = (1u << 31) | (bus << 16) | (dev << 11) | (func << 8);
        /* Read vendor/device ID */
        outl(PCI_CONFIG_ADDR, addr | 0x00);
        uint32_t id = inl(PCI_CONFIG_DATA);
        if ((id & 0xFFFF) == 0xFFFF) {
          if (func == 0) break;
          continue;
        }
        /* Read class/subclass (offset 0x08) */
        outl(PCI_CONFIG_ADDR, addr | 0x08);
        uint32_t classreg = inl(PCI_CONFIG_DATA);
        uint8_t class_code = (uint8_t)(classreg >> 24);
        uint8_t subclass = (uint8_t)(classreg >> 16);

        /* Class 0x03 = Display controller, subclass 0x00 = VGA compatible */
        if (class_code == 0x03 && subclass == 0x00) {
          /* Save PCI address for later BAR dump */
          vga_pci_addr = addr;
          vga_pci_id = id;

          /* Enable PCI memory space access (command register bit 1) */
          outl(PCI_CONFIG_ADDR, addr | 0x04);
          uint16_t cmd = inw(PCI_CONFIG_DATA);
          cmd |= 0x02;
          outw(PCI_CONFIG_ADDR, addr | 0x04);
          outw(PCI_CONFIG_DATA, cmd);

          /* Try to initialize Bochs VBE graphics mode */
          int vbe_ok = vga_init_vbe(1024, 768, 32);
          (void)vbe_ok;

          /* Read BAR0 (offset 0x10) — VGA framebuffer for standard/Bochs VGA */
          outl(PCI_CONFIG_ADDR, addr | 0x10);
          uint32_t bar0 = inl(PCI_CONFIG_DATA);
          if (bar0 != 0 && bar0 != 0xFFFFFFFF && (bar0 & 1) == 0) {
            uint64_t fb = (uint64_t)(bar0 & 0xFFFFFFF0u);
            vga_fb_info.framebuffer = fb;
            vga_fb_info.width = 1024;
            vga_fb_info.height = 768;
            vga_fb_info.pitch = 1024 * 4;
            vga_fb_info.valid = 1;
            return;
          }

          /* Fallback: BAR2 (offset 0x18) */
          outl(PCI_CONFIG_ADDR, addr | 0x18);
          uint32_t bar2 = inl(PCI_CONFIG_DATA);
          if (bar2 != 0 && bar2 != 0xFFFFFFFF && (bar2 & 1) == 0) {
            vga_fb_info.framebuffer = (uint64_t)(bar2 & 0xFFFFFFF0u);
            vga_fb_info.width = 1024;
            vga_fb_info.height = 768;
            vga_fb_info.pitch = 1024 * 4;
            vga_fb_info.valid = 1;
            return;
          }
        }
      }
    }
  }
}

static void __attribute__((noinline)) kernel_stack_entry(void *gop)
{
  uint64_t new_rsp = (uint64_t)(uintptr_t)&kernel_stack[KERNEL_STACK_SIZE];
  __asm__ __volatile__("mov %0, %%rsp" : : "r"(new_rsp) : "memory");
  extern void brights_kernel_main(void *gop);
  brights_kernel_main(gop);
  for (;;) { __asm__ __volatile__("hlt"); }
}

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

static void *uefi_find_gop(EFI_SYSTEM_TABLE *st, EFI_HANDLE image)
{
  static const EFI_GUID gop_guid = EFI_GOP_GUID;
  (void)image;

  void *gop = 0;

  /* Method 1: LocateProtocol (searches all handles for the protocol) */
  if (st->BootServices->LocateProtocol((void *)&gop_guid, 0, &gop) == EFI_SUCCESS && gop) {
    return gop;
  }

  gop = 0;

  /* Method 2: HandleProtocol on image handle */
  typedef EFI_STATUS (*handle_protocol_fn)(EFI_HANDLE, void *, void **);
  handle_protocol_fn hp = (handle_protocol_fn)(uintptr_t)st->BootServices->HandleProtocol;
  if (hp(image, (void *)&gop_guid, &gop) == EFI_SUCCESS && gop) {
    return gop;
  }

  gop = 0;

  /* Method 3: ConfigurationTable search */
  for (uint64_t i = 0; i < st->NumberOfTableEntries; ++i) {
    if (guid_eq(&st->ConfigurationTable[i].VendorGuid, &gop_guid)) {
      return st->ConfigurationTable[i].VendorTable;
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

  void *gop = uefi_find_gop(SystemTable, ImageHandle);
  if (gop) {
    uefi_print_str(&serial_con, u"gop: found\r\n");
  } else {
    uefi_print_str(&serial_con, u"gop: not found\r\n");
  }

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

  /* Detect PCI VGA framebuffer (GOP fallback for OVMF without GOP driver) */
  detect_pci_vga_fb();
  if (vga_fb_info.valid) {
    uefi_print_str(&serial_con, u"vga: fb=");
    uefi_print_hex(&serial_con, vga_fb_info.framebuffer);
    uefi_print_nl(&serial_con);
  }

  brights_vm_init();
  uefi_print_str(&serial_con, u"BrightS kernel: vm ok\r\n");

  if (vga_fb_info.valid) {
    uint64_t fb = vga_fb_info.framebuffer;
    int remap_ok = brights_paging_remap_uc(fb, fb, 1024 * 768 * 4);

    if (remap_ok == 0) {
      uefi_print_str(&serial_con, u"vga: remap ok\r\n");
    } else {
      uefi_print_str(&serial_con, u"vga: remap failed\r\n");
    }

    /* Re-enable VBE mode with correct register indices */
    vbe_write(VBE_DISPI_INDEX_ENABLE, 0);
    vbe_write(VBE_DISPI_INDEX_XRES, 1024);
    vbe_write(VBE_DISPI_INDEX_YRES, 768);
    vbe_write(VBE_DISPI_INDEX_BPP, 32);
    vbe_write(VBE_DISPI_INDEX_VIRT_WIDTH, 1024);
    vbe_write(VBE_DISPI_INDEX_VIRT_HEIGHT, 768);
    vbe_write(VBE_DISPI_INDEX_X_OFFSET, 0);
    vbe_write(VBE_DISPI_INDEX_Y_OFFSET, 0);
    vbe_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);
  }

  {
    uint32_t cpuid_eax, cpuid_ebx, cpuid_ecx, cpuid_edx;

    /* Check CPUID.1:ECX for SMEP (bit 7) and SMAP (bit 27) */
    __asm__ __volatile__(
      "cpuid"
      : "=a"(cpuid_eax), "=b"(cpuid_ebx), "=c"(cpuid_ecx), "=d"(cpuid_edx)
      : "a"(1), "c"(0)
    );

    uint64_t cr4;
    __asm__ __volatile__("mov %%cr4, %0" : "=r"(cr4));

    if (cpuid_ecx & (1u << 7)) {
      cr4 |= (1ULL << 20); /* SMEP */
      uefi_print_str(&serial_con, u"  SMEP supported, enabling\r\n");
    } else {
      uefi_print_str(&serial_con, u"  SMEP not supported, skipping\r\n");
    }
    if (cpuid_ecx & (1u << 27)) {
      cr4 |= (1ULL << 21); /* SMAP */
      uefi_print_str(&serial_con, u"  SMAP supported, enabling\r\n");
    } else {
      uefi_print_str(&serial_con, u"  SMAP not supported, skipping\r\n");
    }

    __asm__ __volatile__("mov %0, %%cr4" :: "r"(cr4) : "memory");
    uefi_print_str(&serial_con, u"cr4: basic features enabled\r\n");
  }

   brights_idt_init();
   uefi_print_str(&serial_con, u"BrightS kernel: idt ok\r\n");
   brights_syscall_abi_init();
   uefi_print_str(&serial_con, u"BrightS kernel: syscall/vfs ok\r\n");
   brights_boot_fs_init();
   kernel_stack_entry(gop);

  for (;;) {
    __asm__ __volatile__("hlt");
  }
  return 0;
}
