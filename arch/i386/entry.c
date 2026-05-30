#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "pit.h"
#include "trap_console.h"
#include "syscall_abi.h"
#include "paging.h"
#include "../../drivers/serial.h"
#include "../../kernel/core/printf.h"
#include "../../kernel/core/kernel_main.h"
#include "../../kernel/core/clock.h"
#include "../../kernel/core/vm.h"
#include "../../kernel/core/acpi.h"
#include "../../kernel/fs/boot_fs.h"

void brights_arch_entry(void)
{
  brights_serial_init(BRIGHTS_COM1_PORT);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "BrightS i386: serial init ok\r\n");

  brights_gdt_init();
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "BrightS i386: gdt init ok\r\n");

  brights_pic_remap();
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "BrightS i386: pic remap ok\r\n");

  brights_idt_init();
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "BrightS i386: idt init ok\r\n");

  brights_pit_init(100);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "BrightS i386: pit init ok (100Hz)\r\n");

  brights_boot_fs_init();
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "BrightS i386: vfs init ok\r\n");

  brights_vm_init();
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "BrightS i386: vm init ok\r\n");

  brights_paging_enable();
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "BrightS i386: paging enabled\r\n");

  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "BrightS i386: calling kernel_main...\r\n");

  brights_kernel_main(0);

  for (;;)
  {
    __asm__ __volatile__("hlt");
  }
}
