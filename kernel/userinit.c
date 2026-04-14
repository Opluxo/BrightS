#include "userinit.h"
#include "elf.h"
#include "proc.h"
#include "sched.h"
#include "printf.h"
#include "fs/ramfs.h"
#include "fs/vfs.h"
#include "fs/vfs2.h"
#include "fs/ramfs_vfs.h"
#include "fs/devfs_vfs.h"
#include "../drivers/serial.h"
#include "../arch/x86_64/gdt.h"
#include "../arch/x86_64/tss.h"
#include "../arch/x86_64/cpu_local.h"
#include "../arch/x86_64/paging.h"

extern void brights_enter_user(void *rip, void *rsp, uint64_t cr3);

static int load_file_to_buffer(const char *path, void *buf, uint64_t buf_size, uint64_t *out_size)
{
  int fd = brights_ramfs_open(path);
  if (fd < 0) return -1;
  int64_t n = brights_ramfs_read(fd, 0, buf, buf_size);
  if (n <= 0) return -1;
  *out_size = (uint64_t)n;
  return 0;
}

/* Total ELF size: ~512 bytes should be enough for a minimal shell */
static const uint8_t embedded_init_elf[512] = {0};

void brights_userinit_enter(void)
{
  brights_console_t con;
  brights_serial_console_init(&con, BRIGHTS_COM1_PORT);

  /* Try to load ELF from filesystem first */
  static uint8_t elf_buf[65536];
  uint64_t elf_size = 0;
  elf64_load_info_t load_info;

  const void *elf_data = embedded_init_elf;
  uint64_t elf_data_size = sizeof(embedded_init_elf);

  if (load_file_to_buffer("/bin/init", elf_buf, sizeof(elf_buf), &elf_size) == 0) {
    elf_data = elf_buf;
    elf_data_size = elf_size;
    brights_print(&con, u"userinit: loaded /bin/init from fs\r\n");
  } else {
    brights_print(&con, u"userinit: using embedded init\r\n");
  }

  /* Load the ELF */
  brights_print(&con, u"userinit_enter: loading ELF...\r\n");
  if (brights_elf_load(elf_data, elf_data_size, &load_info) != 0) {
    brights_print(&con, u"userinit_enter: ELF load failed\r\n");
    return;
  }
  brights_print(&con, u"userinit_enter: ELF loaded\r\n");

  /* Create user process */
  brights_print(&con, u"userinit_enter: spawning process...\r\n");
  int pid = brights_proc_spawn_user("init", load_info.entry, load_info.stack_top);
  if (pid <= 0) {
    brights_print(&con, u"userinit_enter: spawn failed\r\n");
    return;
  }
  brights_print(&con, u"userinit_enter: process spawned\r\n");

  brights_sched_add_process((uint32_t)pid);

  /* Get the CR3 for the user process */
  uint64_t user_cr3 = brights_proc_get_cr3((uint32_t)pid);
  if (user_cr3 == 0) {
    brights_print(&con, u"userinit_enter: failed to get user CR3\r\n");
    return;
  }

  brights_print(&con, u"userinit: entering user mode\r\n");

  /* Enter user mode via iretq */
  brights_enter_user((void *)(uintptr_t)load_info.entry,
                      (void *)(uintptr_t)load_info.stack_top,
                      user_cr3);

  /* Not reached */
}

void brights_userinit(void)
{
  brights_console_t con;
  brights_serial_console_init(&con, BRIGHTS_COM1_PORT);

  /* Initialize VFS2 */
  brights_vfs2_init();
  vfs_superblock_t *ramfs_sb = brights_ramfs_vfs_sb();
  brights_vfs2_mount("/", ramfs_sb);
  brights_devfs_vfs_init();
  vfs_superblock_t *devfs_sb = brights_devfs_vfs_sb();
  brights_vfs2_mount("/dev", devfs_sb);
  brights_print(&con, u"userinit: vfs2 ready\r\n");

  /* Set up GDT/TSS for user mode transitions */
  static uint8_t kstack_buf[8192] __attribute__((aligned(16)));
  uint64_t kstack_top = (uint64_t)(uintptr_t)&kstack_buf[sizeof(kstack_buf)];
  brights_tss_init(kstack_top);
  brights_gdt_init();
  brights_cpu_local_init(kstack_top);

  brights_print(&con, u"userinit: gdt/tss ready\r\n");
  brights_print(&con, u"userinit: use 'bst procom enter-user' to start user shell\r\n");
}