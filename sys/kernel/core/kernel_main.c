
/*
                          _ooOoo_
                         o8888888o
                         88" . "88
                         (| -_- |)
                         O\  =  /O
                      ____/`---'\____
                    .'  \\|     |//  `.
                   /  \\|||  :  |||//  \
                  /  _||||| -:- |||||-  \
                  |   | \\\  -  /// |   |
                  | \_|  ''\---/''  |   |
                  \  .-\__  `-`  ___/-. /
                ___`. .'  /--.--\  `. . __
             ."" '<  `.___\_<|>_/___.'  >'"".
            | | :  `- \`.;`\ _ /`;.`/ - ` : | |
            \  \ `-.   \_ __\ /__ _/   .-` /  /
       ======`-.____`-.___\_____/___.-`____.-'======
                          `=---='
       ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
                佛祖保佑       永无BUG
       */


#include "../dev/serial.h"
#include "../core/printf.h"
#include "acpi.h"
#include "../dev/ramdisk.h"
#include "../fs/vfs.h"
#include "../fs/devfs.h"
#include "../dev/tty.h"
#include "../dev/rtc.h"
#include "../dev/ps2kbd.h"
#include "clock.h"
#include "hwinfo.h"
#include "kmalloc.h"
#include "pmem.h"
#include "proc.h"
#include "sched.h"
#include "signal.h"
#include "sleep.h"
#include "storage.h"
#include "kshell.h"
#include <stddef.h>

void brights_kernel_main(void)
{
  brights_console_t con;
  brights_serial_console_init(&con, BRIGHTS_COM1_PORT);
  brights_tty_init();

  brights_clock_init();
  brights_ps2kbd_init();
  brights_hwinfo_init();
  if (brights_acpi_init() == 0) {
    brights_print(&con, u"acpi: init ok\r\n");
  } else {
    brights_print(&con, u"acpi: init failed\r\n");
  }
  brights_kmalloc_init();
  brights_proc_init();
  brights_sched_init();
  brights_signal_init();

  int dev_count = brights_devfs_init();
  if (dev_count >= 0) {
    brights_print(&con, u"devfs: init ok\r\n");
  } else {
    brights_print(&con, u"devfs: init failed\r\n");
  }

  int kpid = brights_proc_spawn_kernel();
  if (kpid > 0) {
    brights_proc_set_state((uint32_t)kpid, BRIGHTS_PROC_RUNNING);
    brights_sched_mark_dispatch();
    brights_print(&con, u"proc: init ok\r\n");
  } else {
    brights_print(&con, u"proc: init failed\r\n");
  }

  void *km = brights_kmalloc(64);
  if (km) {
    brights_print(&con, u"kmalloc: init ok\r\n");
  } else {
    brights_print(&con, u"kmalloc: init failed\r\n");
  }
  if (brights_pmem_total_bytes() >= (uint64_t)(1024 * 1024)) {
    brights_print(&con, u"pmem: detected\r\n");
  } else {
    brights_print(&con, u"pmem: fallback\r\n");
  }

  // Short calibrated pause to exercise early sleep path.
  brights_sleep_cycles(1024);
  brights_clock_advance(1024);
  brights_sched_tick();

  brights_storage_bootstrap();
  if (brights_storage_backend()[0] == 'n') {
    brights_print(&con, u"nvme: init ok\r\n");
  } else if (brights_storage_backend()[0] == 'a') {
    brights_print(&con, u"ahci: init ok\r\n");
  } else {
    brights_print(&con, u"ramdisk: fallback\r\n");
  }

  if (brights_storage_mount_system() == 0) {
    brights_print(&con, u"storage: mounted at /mnt/drive/\r\n");
  } else {
    brights_print(&con, u"storage: mount failed (system disk must be btrfs)\r\n");
    for (;;) {
      __asm__ __volatile__("hlt");
    }
  }

  brights_rtc_time_t rt;
  if (brights_rtc_read(&rt) == 0) {
    brights_print(&con, u"rtc: init ok\r\n");
  } else {
    brights_print(&con, u"rtc: init failed\r\n");
  }

  brights_kshell_run();
}
