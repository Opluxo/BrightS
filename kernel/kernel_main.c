
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

#include "../drivers/serial.h"
#include "printf.h"
#include "acpi.h"
#include "vmware.h"
#include "../drivers/ramdisk.h"
#include "fs/vfs.h"
#include "fs/devfs.h"
#include "../drivers/tty.h"
#include "../drivers/rtc.h"
#include "../drivers/ps2kbd.h"
#include "../drivers/fb.h"
#include "../drivers/font.h"
#include "clock.h"
#include "hwinfo.h"
#include "kmalloc.h"
#include "../../../include/kernel/cache.h"
#include "pmem.h"
#include "proc.h"
#include "sched.h"
#include "signal.h"
#include "syshook.h"
#include "sleep.h"
#include "storage.h"
#include "lightshell.h"
#include "userinit.h"
#include "smp.h"
#include "../arch/x86_64/apic.h"
#include "../arch/x86_64/ioapic.h"
#include "../arch/x86_64/hpet.h"
#include "../arch/x86_64/mtrr.h"
#include "../net/net.h"
#include "../net/virtionet.h"
#include "kernel_main.h"

/* Helper to print a uint64_t as decimal */
static void print_u64(brights_console_t *con, uint64_t val)
{
  char buf[24];
  int i = 0;
  if (val == 0) { buf[i++] = '0'; }
  else { uint64_t v = val; while (v > 0) { buf[i++] = '0' + (v % 10); v /= 10; } }
  for (int j = 0; j < i/2; ++j) { char t = buf[j]; buf[j] = buf[i-1-j]; buf[i-1-j] = t; }
  buf[i] = 0;
  for (int j = 0; buf[j]; ++j) { char cs[2] = {buf[j], 0}; brights_serial_write_ascii(BRIGHTS_COM1_PORT, cs); }
}

void brights_kernel_main(void *gop)
{
  brights_console_t con;
  brights_serial_console_init(&con, BRIGHTS_COM1_PORT);
  brights_print(&con, u"DEBUG: Kernel main started\r\n");
  brights_tty_init();

  if (gop && brights_fb_init(gop) == 0) {
    brights_print(&con, u"fb: initialized\r\n");
    brights_fb_clear((brights_color_t){0, 0, 40, 255});
    brights_font_draw_string(10, 10, "BrightS OS", 
      (255 << 16) | (200 << 8) | 50,
      0xFFFFFFFF);
  } else {
    brights_print(&con, u"fb: not available\r\n");
  }

  brights_clock_init();
  brights_ps2kbd_init();
  brights_hwinfo_init();

  /* ---- CPU Info ---- */
  const brights_cpu_info_t *cpu = brights_hwinfo_cpu();
  brights_print(&con, u"cpu: ");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, cpu->vendor);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, " ");
  print_u64(&con, cpu->family); brights_serial_write_ascii(BRIGHTS_COM1_PORT, ".");
  print_u64(&con, cpu->model);  brights_serial_write_ascii(BRIGHTS_COM1_PORT, ".");
  print_u64(&con, cpu->stepping);
  brights_print(&con, u"\r\n");

  /* Print CPU features */
  brights_print(&con, u"cpu: ");
  if (cpu->has_sse42) brights_serial_write_ascii(BRIGHTS_COM1_PORT, "SSE4.2 ");
  if (cpu->has_avx)   brights_serial_write_ascii(BRIGHTS_COM1_PORT, "AVX ");
  if (cpu->has_avx2)  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "AVX2 ");
  if (cpu->has_rdrand) brights_serial_write_ascii(BRIGHTS_COM1_PORT, "RDRAND ");
  if (cpu->has_x2apic) brights_serial_write_ascii(BRIGHTS_COM1_PORT, "x2APIC ");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\r\n");

  /* Print cache info */
  brights_print(&con, u"cpu: L1d=");
  print_u64(&con, cpu->l1d_size / 1024);
  brights_print(&con, u"KB L1i=");
  print_u64(&con, cpu->l1i_size / 1024);
  brights_print(&con, u"KB L2=");
  print_u64(&con, cpu->l2_size / 1024);
  brights_print(&con, u"KB L3=");
  print_u64(&con, cpu->l3_size / (1024 * 1024));
  brights_print(&con, u"MB\r\n");

  /* Print core topology */
  brights_print(&con, u"cpu: ");
  print_u64(&con, cpu->cores_per_pkg);
  brights_print(&con, u" cores, ");
  print_u64(&con, cpu->logical_cores);
  brights_print(&con, u" threads\r\n");

  /* Calibrate TSC */
  brights_hwinfo_calibrate_tsc();
  brights_print(&con, u"tsc: freq=");
  print_u64(&con, cpu->tsc_freq / 1000000);
  brights_print(&con, u" MHz");
  if (cpu->tsc_invariant) brights_serial_write_ascii(BRIGHTS_COM1_PORT, " (invariant)");
  brights_print(&con, u"\r\n");

  /* ---- ACPI ---- */
  if (brights_acpi_init() == 0) {
    brights_print(&con, u"acpi: init ok\r\n");
    const brights_acpi_info_t *ai = brights_acpi_info();
    if (ai->has_madt) {
      brights_print(&con, u"acpi: madt lapic=");
      brights_print(&con, u"0x");
      /* Print LAPIC address */
      char hex[20]; int h = 0;
      uint64_t v = ai->lapic_mmio;
      const char *hxdig = "0123456789ABCDEF";
      for (int i = 0; i < 16; ++i) hex[h++] = hxdig[(v >> (60-i*4)) & 0xF];
      hex[h] = 0; brights_serial_write_ascii(BRIGHTS_COM1_PORT, hex);
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, " ioapic=");
      print_u64(&con, ai->ioapic_count);
      brights_print(&con, u"\r\n");
    }
  } else {
    brights_print(&con, u"acpi: init failed\r\n");
  }

  /* ---- APIC ---- */
  if (brights_apic_init() == 0) {
    brights_print(&con, u"apic: init ok (id=");
    print_u64(&con, brights_apic_id());
    brights_print(&con, u")\r\n");

    /* Calibrate APIC timer */
    brights_apic_timer_calibrate();
    brights_print(&con, u"apic: timer freq=");
    print_u64(&con, brights_apic_timer_freq() / 1000000);
    brights_print(&con, u" MHz\r\n");
  } else {
    brights_print(&con, u"apic: init failed, using PIT\r\n");
  }

  /* ---- IOAPIC ---- */
  {
    const brights_acpi_info_t *ai = brights_acpi_info();
    if (ai->ioapic_count > 0 && ai->ioapic[0].mmio_addr != 0) {
      if (brights_ioapic_init(ai->ioapic[0].mmio_addr) == 0) {
        brights_print(&con, u"ioapic: init ok (gsi=");
        print_u64(&con, brights_ioapic_gsi_count());
        brights_print(&con, u")\r\n");

        /* Route IRQ0 (timer) → vector 32, IRQ1 (keyboard) → vector 33 */
        brights_ioapic_set_redirect(0, 0x20, 0, brights_apic_id());
        brights_ioapic_set_redirect(1, 0x21, 0, brights_apic_id());
        brights_ioapic_unmask(0);
        brights_ioapic_unmask(1);
        brights_print(&con, u"ioapic: irq0/irq1 routed\r\n");
      } else {
        brights_print(&con, u"ioapic: init failed\r\n");
      }
    }
  }

  /* ---- HPET ---- */
  {
    const brights_acpi_info_t *ai = brights_acpi_info();
    if (ai->has_hpet && ai->hpet_mmio != 0) {
      if (brights_hpet_init(ai->hpet_mmio) == 0) {
        brights_print(&con, u"hpet: init ok (freq=");
        print_u64(&con, brights_hpet_freq() / 1000000);
        brights_print(&con, u" MHz)\r\n");
      } else {
        brights_print(&con, u"hpet: init failed\r\n");
      }
    }
  }

  /* ---- MTRR/PAT ---- */
  brights_mtrr_init();
  brights_pat_init();
  brights_print(&con, u"mtrr: ");
  print_u64(&con, brights_mtrr_count());
  brights_print(&con, u" entries, pat configured\r\n");

  /* ---- SMP ---- */
  if (brights_apic_available()) {
    brights_smp_init();
  }

  /* ---- Memory ---- */
  brights_kmalloc_init();
  brights_cache_init();
  brights_proc_init();
  brights_sched_init();
  brights_signal_init();
  brights_syshook_init();
   brights_print(&con, u"syshook: init ok (32 hook slots)\r\n");
   brights_print(&con, u"DEBUG: Syshook initialized\r\n");

  brights_print(&con, u"pmem: ");
  print_u64(&con, brights_pmem_total_bytes() / (1024 * 1024));
  brights_print(&con, u" MB total, ");
  print_u64(&con, brights_pmem_free_bytes() / (1024 * 1024));
  brights_print(&con, u" MB free (");
  print_u64(&con, brights_pmem_free_pages_count());
  brights_print(&con, u" pages)\r\n");

  void *km = brights_kmalloc(64);
  if (km) {
    brights_print(&con, u"kmalloc: init ok (8MB heap)\r\n");
    brights_kfree(km);
  } else {
    brights_print(&con, u"kmalloc: init failed\r\n");
  }

  /* ---- Scheduler timer ---- */
  /* Use APIC timer if available, otherwise PIT handles it via IDT */
  if (brights_apic_available()) {
    brights_apic_timer_init(100); /* 100 Hz scheduling tick */
    brights_print(&con, u"sched: APIC timer @ 100Hz\r\n");
  } else {
    brights_print(&con, u"sched: PIT timer @ 100Hz\r\n");
  }

  /* ---- Process system ---- */
  int dev_count = brights_devfs_init();
  if (dev_count >= 0) {
    brights_print(&con, u"devfs: init ok\r\n");
  }

  int kpid = brights_proc_spawn_kernel("init");
  if (kpid > 0) {
    brights_proc_set_state((uint32_t)kpid, BRIGHTS_PROC_RUNNING);
    brights_proc_set_current((uint32_t)kpid);
    brights_sched_add_process((uint32_t)kpid);
    brights_sched_mark_dispatch();
    brights_print(&con, u"proc: init ok\r\n");
  }

  /* ---- Storage ---- */
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
    brights_print(&con, u"storage: continuing with limited functionality\r\n");
    brights_print(&con, u"DEBUG: About to init RTC\r\n");
  }

  /* ---- RTC ---- */
  brights_vmware_backdoor_init();
  brights_print(&con, u"rtc: checking...\r\n");
  brights_rtc_time_t rt;
  brights_print(&con, u"DEBUG: About to call rtc_read\r\n");
  if (brights_rtc_read(&rt) == 0) {
    brights_print(&con, u"rtc: ");
    /* Print date/time */
    char dt[32]; int d = 0;
    dt[d++] = '2'; dt[d++] = '0';
    dt[d++] = '0' + (rt.year / 10) % 10; dt[d++] = '0' + rt.year % 10;
    dt[d++] = '-';
    dt[d++] = '0' + (rt.month / 10) % 10; dt[d++] = '0' + rt.month % 10;
    dt[d++] = '-';
    dt[d++] = '0' + (rt.day / 10) % 10; dt[d++] = '0' + rt.day % 10;
    dt[d++] = ' ';
    dt[d++] = '0' + (rt.hour / 10) % 10; dt[d++] = '0' + rt.hour % 10;
    dt[d++] = ':';
    dt[d++] = '0' + (rt.minute / 10) % 10; dt[d++] = '0' + rt.minute % 10;
    dt[d++] = ':';
    dt[d++] = '0' + (rt.second / 10) % 10; dt[d++] = '0' + rt.second % 10;
    dt[d++] = '\r'; dt[d++] = '\n'; dt[d] = 0;
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, dt);
  }

  /* ---- VFS2 + User mode ---- */
  brights_print(&con, u"DEBUG: About to init user mode\r\n");
  brights_print(&con, u"user: initializing...\r\n");
  brights_userinit();
  brights_print(&con, u"DEBUG: userinit() returned\r\n");

  /* ---- Network (simplified for faster boot) ---- */
  brights_print(&con, u"net: init...\r\n");
  brights_net_init();
  brights_virtionet_init();
  {
    uint8_t mac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    brights_netif_add("eth0", mac);
    brights_netif_set_ip("eth0", 0xC0A80164, 0xFFFFFF00, 0xC0A80101);
    brights_netif_up("eth0");
  }
  brights_print(&con, u"net: ready\r\n");

  /* ---- Enable interrupts ---- */
  __asm__ __volatile__("sti");
  brights_print(&con, u"interrupts: enabled\r\n");

  /* ---- Shell (should be last) ---- */
  brights_print(&con, u"shell: starting...\r\n");
  brights_print(&con, u"\r\n--- BrightS kernel ready ---\r\n\r\n");

  brights_lightshell_run();
}
