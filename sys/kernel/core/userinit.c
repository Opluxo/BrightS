#include "userinit.h"
#include "../fs/ramfs.h"
#include "../core/printf.h"
#include "../dev/serial.h"
#include "../platform/x86_64/gdt.h"
#include "../platform/x86_64/tss.h"
#include "../platform/x86_64/cpu_local.h"

extern void brights_enter_user(void *rip, void *rsp);
extern void brights_user_start(void);

static uint8_t user_stack[4096];
static uint8_t kernel_stack[4096];

void brights_userinit(void)
{
  brights_console_t con;
  brights_serial_console_init(&con, BRIGHTS_COM1_PORT);

  // Prepare userspace system file in /sys.
  int fd = brights_ramfs_open("/sys/init.rc");
  if (fd < 0) {
    brights_print(&con, u"userinit: /sys/init.rc not found\r\n");
  }

  uint64_t kstack_top = (uint64_t)(uintptr_t)&kernel_stack[sizeof(kernel_stack)];
  brights_tss_init(kstack_top);
  brights_gdt_init();
  brights_cpu_local_init(kstack_top);

  uint64_t ustack_top = (uint64_t)(uintptr_t)&user_stack[sizeof(user_stack)];
  brights_print(&con, u"userinit: enter user\r\n");
  brights_enter_user((void *)brights_user_start, (void *)ustack_top);
}
