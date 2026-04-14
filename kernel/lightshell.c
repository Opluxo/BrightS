#include "lightshell.h"
#include "lightshell_cmds/netget.h"
#include "../drivers/serial.h"
#include "../arch/x86_64/io.h"
#include "fs/ramfs.h"
#include "fs/btrfs.h"
#include "fs/vfs.h"
#include "../net/net.h"
#include "../net/wifi.h"
#include "../drivers/rtc.h"
#include "../drivers/ps2kbd.h"
#include "../drivers/tty.h"
#include "clock.h"
#include "acpi.h"
#include "hwinfo.h"
#include "kmalloc.h"
#include "pmem.h"
#include "proc.h"
#include "sched.h"
#include "signal.h"
#include "syshook.h"
#include "sleep.h"
#include "storage.h"
#include "kernel_util.h"
#include "userinit.h"
#include "smp.h"
#include "../arch/x86_64/apic.h"
#include "../arch/x86_64/ioapic.h"
#include "../arch/x86_64/hpet.h"
#include "../arch/x86_64/mtrr.h"
#include <stdint.h>

/* ===== Command dispatch table (binary search) ===== */
typedef struct {
  const char *name;
  int (*handler)(const char *arg);
} cmd_entry_t;

/* Forward declarations for command handlers */
static int cmd_help_handler(const char *arg);
static int cmd_ls_handler(const char *arg);
static int cmd_pwd_handler(const char *arg);
static int cmd_cd_handler(const char *arg);
static int cmd_mkdir_handler(const char *arg);
static int cmd_rmdir_handler(const char *arg);
static int cmd_cat_handler(const char *arg);
static int cmd_stat_handler(const char *arg);
static int cmd_rm_handler(const char *arg);
static int cmd_cp_handler(const char *arg);
static int cmd_mv_handler(const char *arg);
static int cmd_echo_handler(const char *arg);
static int cmd_kill_handler(const char *arg);
static int cmd_jobs_handler(const char *arg);
static int cmd_wifi_handler(const char *arg);
static int cmd_ifconfig_handler(const char *arg);
static int cmd_bst_handler(const char *arg);
static int cmd_login_handler(const char *arg);
static int cmd_logout_handler(const char *arg);
static int cmd_whoami_handler(const char *arg);
static int cmd_profile_handler(const char *arg);
static int cmd_passwd_handler(const char *arg);
static int cmd_useradd_handler(const char *arg);
static int cmd_setpf_handler(const char *arg);
static int cmd_touch_handler(const char *arg);
static int cmd_write_handler(const char *arg);
static int cmd_append_handler(const char *arg);
static int cmd_hexdump_handler(const char *arg);
static int cmd_uname_handler(const char *arg);
static int cmd_mount_handler(const char *arg);
static int cmd_clear_handler(const char *arg);
static int cmd_reboot_handler(const char *arg);
static int cmd_halt_handler(const char *arg);
static int cmd_date_handler(const char *arg);

/* Sorted command table for binary search */
static const cmd_entry_t cmd_table[] = {
  {"append",  cmd_append_handler},
  {"bst",     cmd_bst_handler},
  {"cat",     cmd_cat_handler},
  {"cd",      cmd_cd_handler},
  {"clear",   cmd_clear_handler},
  {"cp",      cmd_cp_handler},
  {"date",    cmd_date_handler},
  {"echo",    cmd_echo_handler},
  {"halt",    cmd_halt_handler},
  {"help",    cmd_help_handler},
  {"hexdump", cmd_hexdump_handler},
  {"ifconfig",cmd_ifconfig_handler},
  {"jobs",    cmd_jobs_handler},
  {"kill",    cmd_kill_handler},
  {"login",   cmd_login_handler},
  {"logout",  cmd_logout_handler},
  {"ls",      cmd_ls_handler},
  {"mkdir",   cmd_mkdir_handler},
  {"mount",   cmd_mount_handler},
  {"netget",  cmd_netget_handler},
  {"mv",      cmd_mv_handler},
  {"passwd",  cmd_passwd_handler},
  {"profile", cmd_profile_handler},
  {"pwd",     cmd_pwd_handler},
  {"reboot",  cmd_reboot_handler},
  {"rm",      cmd_rm_handler},
  {"rmdir",   cmd_rmdir_handler},
  {"setpf",   cmd_setpf_handler},
  {"stat",    cmd_stat_handler},
  {"touch",   cmd_touch_handler},
  {"uname",   cmd_uname_handler},
  {"useradd", cmd_useradd_handler},
  {"whoami",  cmd_whoami_handler},
  {"wifi",    cmd_wifi_handler},
  {"write",   cmd_write_handler},
};

static const int cmd_count = sizeof(cmd_table) / sizeof(cmd_table[0]);

/* Binary search for command lookup - O(log n) instead of O(n) */
static const cmd_entry_t *cmd_find(const char *name)
{
  int lo = 0, hi = cmd_count - 1;
  while (lo <= hi) {
    int mid = (lo + hi) / 2;
    int cmp = kutil_strcmp(cmd_table[mid].name, name);
    if (cmp == 0) return &cmd_table[mid];
    if (cmp < 0) lo = mid + 1;
    else hi = mid - 1;
  }
  return 0;
}

#define LIGHTSHELL_MAX_LINE 256
#define LIGHTSHELL_MAX_USER 32
#define LIGHTSHELL_MAX_PASS 64
#define LIGHTSHELL_MAX_CFG  1024
#define LIGHTSHELL_MAX_PATH 128
#define LIGHTSHELL_HISTORY_SIZE 32

static char current_user[LIGHTSHELL_MAX_USER] = "guest";
static char current_dir[LIGHTSHELL_MAX_PATH] = "/";
static int is_root = 0;
static char version[20] = "0.0.5 Beta";

// Command history
static char history[LIGHTSHELL_HISTORY_SIZE][LIGHTSHELL_MAX_LINE];
static int history_count = 0;
static int history_index = -1;
static int history_nav_index = -1;

// Command list for tab completion
static const char *commands[] = {
  "help", "ls", "pwd", "cd", "mkdir", "rmdir", "whoami", "profile",
  "logout", "bst", "cat", "stat", "login", "passwd", "useradd", "setpf",
  "touch", "write", "append", "rm", "cp", "mv", "hexdump", "echo",
  "kill", "jobs", "fg", "bg", "netget",
  0
};

static void cmd_date(void);
static void cmd_kbdtest(void);
static void cmd_uname(void);
static void cmd_mount(void);
static void cmd_clear(void);
static int cmd_runuser(void);
static int cmd_reboot(void);
static int cmd_halt(void);
static void cmd_rmdir(const char *arg);
static void cmd_cp(const char *arg);
static void cmd_mv(const char *arg);
static void cmd_kill(const char *arg);
static void cmd_jobs(void);
static void cmd_wifi(const char *arg);
static void cmd_ifconfig(const char *arg);

static int streq(const char *a, const char *b)
{
  while (*a && *b) {
    if (*a != *b) {
      return 0;
    }
    ++a;
    ++b;
  }
  return *a == 0 && *b == 0;
}

static int starts_with(const char *s, const char *prefix)
{
  while (*prefix) {
    if (*s++ != *prefix++) {
      return 0;
    }
  }
  return 1;
}

static int strlen_s(const char *s)
{
  int n = 0;
  while (s[n]) {
    ++n;
  }
  return n;
}

static void str_copy(char *dst, int cap, const char *src)
{
  if (!dst || cap <= 0) {
    return;
  }
  int i = 0;
  while (src && src[i] && i < cap - 1) {
    dst[i] = src[i];
    ++i;
  }
  dst[i] = 0;
}

static int streq_n(const char *a, const char *b, int n)
{
  for (int i = 0; i < n; ++i) {
    if (a[i] != b[i]) {
      return 0;
    }
  }
  return 1;
}

static void print_u64(uint64_t v)
{
  char tmp[24];
  int i = 0;
  if (v == 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "0");
    return;
  }
  while (v > 0 && i < (int)sizeof(tmp)) {
    tmp[i++] = (char)('0' + (v % 10u));
    v /= 10u;
  }
  while (i > 0) {
    char out[2] = {tmp[--i], 0};
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, out);
  }
}

static void print_u2(uint8_t v)
{
  char out[3];
  out[0] = (char)('0' + (v / 10u));
  out[1] = (char)('0' + (v % 10u));
  out[2] = 0;
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, out);
}

static void print_u4(uint16_t v)
{
  char out[5];
  out[0] = (char)('0' + ((v / 1000u) % 10u));
  out[1] = (char)('0' + ((v / 100u) % 10u));
  out[2] = (char)('0' + ((v / 10u) % 10u));
  out[3] = (char)('0' + (v % 10u));
  out[4] = 0;
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, out);
}

static void print_hex8(uint8_t v)
{
  static const char *h = "0123456789ABCDEF";
  char out[3];
  out[0] = h[(v >> 4) & 0xF];
  out[1] = h[v & 0xF];
  out[2] = 0;
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, out);
}

static const char *find_space(const char *s)
{
  while (*s && *s != ' ') {
    ++s;
  }
  return s;
}

static const char *skip_spaces(const char *s)
{
  while (*s == ' ') {
    ++s;
  }
  return s;
}

static int parse_two_args(const char *arg,
                          char *first,
                          int first_cap,
                          char *second,
                          int second_cap)
{
  if (!arg || !first || !second || first_cap <= 1 || second_cap <= 1) {
    return -1;
  }

  arg = skip_spaces(arg);
  int i = 0;
  while (*arg && *arg != ' ' && i < first_cap - 1) {
    first[i++] = *arg++;
  }
  first[i] = 0;
  arg = skip_spaces(arg);

  int j = 0;
  while (*arg && *arg != ' ' && j < second_cap - 1) {
    second[j++] = *arg++;
  }
  second[j] = 0;
  arg = skip_spaces(arg);

  if (first[0] == 0 || second[0] == 0 || *arg != 0) {
    return -1;
  }
  return 0;
}

static int path_normalize(const char *path, char *out, int cap)
{
  if (!path || !out || cap < 2) {
    return -1;
  }

  char segs[16][LIGHTSHELL_MAX_PATH];
  int seg_count = 0;
  int i = 0;
  while (path[i]) {
    while (path[i] == '/') {
      ++i;
    }
    if (!path[i]) {
      break;
    }
    char seg[LIGHTSHELL_MAX_PATH];
    int slen = 0;
    while (path[i] && path[i] != '/') {
      if (slen >= LIGHTSHELL_MAX_PATH - 1) {
        return -1;
      }
      seg[slen++] = path[i++];
    }
    seg[slen] = 0;
    if (slen == 1 && seg[0] == '.') {
      continue;
    }
    if (slen == 2 && seg[0] == '.' && seg[1] == '.') {
      if (seg_count > 0) {
        --seg_count;
      }
      continue;
    }
    if (seg_count >= 16) {
      return -1;
    }
    for (int j = 0; j <= slen; ++j) {
      segs[seg_count][j] = seg[j];
    }
    ++seg_count;
  }

  if (seg_count == 0) {
    out[0] = '/';
    out[1] = 0;
    return 0;
  }

  int p = 0;
  out[p++] = '/';
  for (int s = 0; s < seg_count; ++s) {
    for (int j = 0; segs[s][j]; ++j) {
      if (p >= cap - 1) {
        return -1;
      }
      out[p++] = segs[s][j];
    }
    if (s + 1 < seg_count) {
      if (p >= cap - 1) {
        return -1;
      }
      out[p++] = '/';
    }
  }
  out[p] = 0;
  return 0;
}

static int resolve_path(const char *input, char *out, int cap)
{
  if (!input || !out) {
    return -1;
  }
  input = skip_spaces(input);
  if (*input == 0) {
    return path_normalize(current_dir, out, cap);
  }
  if (*input == '/') {
    return path_normalize(input, out, cap);
  }

  char joined[LIGHTSHELL_MAX_PATH * 2];
  int p = 0;
  for (int i = 0; current_dir[i] && p < (int)sizeof(joined) - 1; ++i) {
    joined[p++] = current_dir[i];
  }
  if (p > 1 && joined[p - 1] != '/') {
    joined[p++] = '/';
  }
  for (int i = 0; input[i] && p < (int)sizeof(joined) - 1; ++i) {
    joined[p++] = input[i];
  }
  joined[p] = 0;
  return path_normalize(joined, out, cap);
}

static int path_basename(const char *path, char *out, int cap)
{
  int start = 0;
  for (int i = 0; path[i]; ++i) {
    if (path[i] == '/') {
      start = i + 1;
    }
  }
  if (!path[start]) {
    if (cap < 2) {
      return -1;
    }
    out[0] = '/';
    out[1] = 0;
    return 0;
  }
  int p = 0;
  for (int i = start; path[i] && p < cap - 1; ++i) {
    out[p++] = path[i];
  }
  out[p] = 0;
  return (p > 0) ? 0 : -1;
}

static int is_direct_child(const char *parent, const char *path)
{
  if (streq(parent, "/")) {
    if (path[0] != '/' || path[1] == 0) {
      return 0;
    }
    for (int i = 1; path[i]; ++i) {
      if (path[i] == '/') {
        return 0;
      }
    }
    return 1;
  }

  int plen = strlen_s(parent);
  for (int i = 0; i < plen; ++i) {
    if (path[i] != parent[i]) {
      return 0;
    }
  }
  if (path[plen] != '/') {
    return 0;
  }
  if (path[plen + 1] == 0) {
    return 0;
  }
  for (int i = plen + 1; path[i]; ++i) {
    if (path[i] == '/') {
      return 0;
    }
  }
  return 1;
}

static int userpf_path(const char *user, char *out, int cap)
{
  const char *prefix = "/bin/config/";
  const char *suffix = "/example.pf";
  int p = 0;
  int ulen = strlen_s(user);
  if (ulen <= 0 || ulen >= LIGHTSHELL_MAX_USER) {
    return -1;
  }
  for (int i = 0; prefix[i] && p < cap - 1; ++i) out[p++] = prefix[i];
  for (int i = 0; user[i] && p < cap - 1; ++i) out[p++] = user[i];
  for (int i = 0; suffix[i] && p < cap - 1; ++i) out[p++] = suffix[i];
  if (p >= cap - 1) {
    return -1;
  }
  out[p] = 0;
  return 0;
}

static int pf_read(const char *user, char *buf, int cap)
{
  char path[128];
  if (userpf_path(user, path, sizeof(path)) < 0) {
    return -1;
  }
  int fd = brights_ramfs_open(path);
  if (fd < 0) {
    return -1;
  }
  int64_t n = brights_ramfs_read(fd, 0, buf, (uint64_t)(cap - 1));
  if (n < 0) {
    return -1;
  }
  buf[n] = 0;
  return (int)n;
}

static int pf_write(const char *user, const char *content, int len)
{
  char path[128];
  if (userpf_path(user, path, sizeof(path)) < 0) {
    return -1;
  }
  int fd = brights_ramfs_open(path);
  if (fd < 0) {
    fd = brights_ramfs_create(path);
  }
  if (fd < 0) {
    return -1;
  }
  return (brights_ramfs_write(fd, 0, content, (uint64_t)len) >= 0) ? 0 : -1;
}

static int pf_extract(const char *buf, const char *key, char *out, int out_cap)
{
  int key_len = strlen_s(key);
  int i = 0;
  while (buf[i]) {
    int line_start = i;
    while (buf[i] && buf[i] != '\n') {
      ++i;
    }
    int line_end = i;
    if (buf[i] == '\n') {
      ++i;
    }
    if (line_end - line_start <= key_len + 1) {
      continue;
    }
    if (!streq_n(&buf[line_start], key, key_len)) {
      continue;
    }
    if (buf[line_start + key_len] != ':') {
      continue;
    }
    int vstart = line_start + key_len + 1;
    int vlen = line_end - vstart;
    int copy = (vlen < out_cap - 1) ? vlen : (out_cap - 1);
    for (int k = 0; k < copy; ++k) {
      out[k] = buf[vstart + k];
    }
    out[copy] = 0;
    return 0;
  }
  return -1;
}

static int pf_write_default(const char *user, const char *pass)
{
  char content[LIGHTSHELL_MAX_CFG];
  const char *host = "brights";
  const char *avatar = "\"default\"";
  const char *email = "user@local";
  int p = 0;
  const char *k1 = "username:";
  for (int i = 0; k1[i]; ++i) content[p++] = k1[i];
  for (int i = 0; user[i]; ++i) content[p++] = user[i];
  content[p++] = '\n';
  const char *k2 = "hostname:";
  for (int i = 0; k2[i]; ++i) content[p++] = k2[i];
  for (int i = 0; host[i]; ++i) content[p++] = host[i];
  content[p++] = '\n';
  const char *k3 = "avatar:";
  for (int i = 0; k3[i]; ++i) content[p++] = k3[i];
  for (int i = 0; avatar[i]; ++i) content[p++] = avatar[i];
  content[p++] = '\n';
  const char *k4 = "email:";
  for (int i = 0; k4[i]; ++i) content[p++] = k4[i];
  for (int i = 0; email[i]; ++i) content[p++] = email[i];
  content[p++] = '\n';
  const char *k5 = "password:";
  for (int i = 0; k5[i]; ++i) content[p++] = k5[i];
  for (int i = 0; pass[i]; ++i) content[p++] = pass[i];
  content[p++] = '\n';
  content[p] = 0;
  return pf_write(user, content, p);
}

static int pf_get_password(const char *user, char *pass_out, int pass_cap)
{
  char cfg[LIGHTSHELL_MAX_CFG];
  if (pf_read(user, cfg, sizeof(cfg)) < 0) {
    return -1;
  }
  return pf_extract(cfg, "password", pass_out, pass_cap);
}

static int pf_exists(const char *user)
{
  char path[128];
  if (userpf_path(user, path, sizeof(path)) < 0) {
    return -1;
  }
  return (brights_ramfs_open(path) >= 0) ? 0 : -1;
}

static int pf_set_password(const char *user, const char *newpass)
{
  char cfg[LIGHTSHELL_MAX_CFG];
  if (pf_read(user, cfg, sizeof(cfg)) < 0) {
    return -1;
  }
  char uname[LIGHTSHELL_MAX_USER];
  char host[64];
  char avatar[64];
  char email[128];
  if (pf_extract(cfg, "username", uname, sizeof(uname)) < 0) {
    str_copy(uname, sizeof(uname), user);
  }
  if (pf_extract(cfg, "hostname", host, sizeof(host)) < 0) {
    str_copy(host, sizeof(host), "brights");
  }
  if (pf_extract(cfg, "avatar", avatar, sizeof(avatar)) < 0) {
    str_copy(avatar, sizeof(avatar), "\"default\"");
  }
  if (pf_extract(cfg, "email", email, sizeof(email)) < 0) {
    str_copy(email, sizeof(email), "user@local");
  }

  char out[LIGHTSHELL_MAX_CFG];
  int p = 0;
  const char *k1 = "username:";
  for (int i = 0; k1[i]; ++i) out[p++] = k1[i];
  for (int i = 0; uname[i]; ++i) out[p++] = uname[i];
  out[p++] = '\n';
  const char *k2 = "hostname:";
  for (int i = 0; k2[i]; ++i) out[p++] = k2[i];
  for (int i = 0; host[i]; ++i) out[p++] = host[i];
  out[p++] = '\n';
  const char *k3 = "avatar:";
  for (int i = 0; k3[i]; ++i) out[p++] = k3[i];
  for (int i = 0; avatar[i]; ++i) out[p++] = avatar[i];
  out[p++] = '\n';
  const char *k4 = "email:";
  for (int i = 0; k4[i]; ++i) out[p++] = k4[i];
  for (int i = 0; email[i]; ++i) out[p++] = email[i];
  out[p++] = '\n';
  const char *k5 = "password:";
  for (int i = 0; k5[i]; ++i) out[p++] = k5[i];
  for (int i = 0; newpass[i]; ++i) out[p++] = newpass[i];
  out[p++] = '\n';
  out[p] = 0;
  return pf_write(user, out, p);
}

static int pf_show(const char *user)
{
  char cfg[LIGHTSHELL_MAX_CFG];
  if (pf_read(user, cfg, sizeof(cfg)) < 0) {
    return -1;
  }
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, cfg);
  if (cfg[0] && cfg[strlen_s(cfg) - 1] != '\n') {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
  }
  return 0;
}

static int pf_set_field(const char *user, const char *key, const char *value)
{
  char cfg[LIGHTSHELL_MAX_CFG];
  if (pf_read(user, cfg, sizeof(cfg)) < 0) {
    return -1;
  }
  char uname[LIGHTSHELL_MAX_USER];
  char host[64];
  char avatar[64];
  char email[128];
  char pass[LIGHTSHELL_MAX_PASS];
  if (pf_extract(cfg, "username", uname, sizeof(uname)) < 0) str_copy(uname, sizeof(uname), user);
  if (pf_extract(cfg, "hostname", host, sizeof(host)) < 0) str_copy(host, sizeof(host), "brights");
  if (pf_extract(cfg, "avatar", avatar, sizeof(avatar)) < 0) str_copy(avatar, sizeof(avatar), "\"default\"");
  if (pf_extract(cfg, "email", email, sizeof(email)) < 0) str_copy(email, sizeof(email), "user@local");
  if (pf_extract(cfg, "password", pass, sizeof(pass)) < 0) str_copy(pass, sizeof(pass), "guest");

  if (streq(key, "hostname")) {
    str_copy(host, sizeof(host), value);
  } else if (streq(key, "avatar")) {
    str_copy(avatar, sizeof(avatar), value);
  } else if (streq(key, "email")) {
    str_copy(email, sizeof(email), value);
  } else {
    return -1;
  }

  char out[LIGHTSHELL_MAX_CFG];
  int p = 0;
  const char *k1 = "username:";
  for (int i = 0; k1[i]; ++i) out[p++] = k1[i];
  for (int i = 0; uname[i]; ++i) out[p++] = uname[i];
  out[p++] = '\n';
  const char *k2 = "hostname:";
  for (int i = 0; k2[i]; ++i) out[p++] = k2[i];
  for (int i = 0; host[i]; ++i) out[p++] = host[i];
  out[p++] = '\n';
  const char *k3 = "avatar:";
  for (int i = 0; k3[i]; ++i) out[p++] = k3[i];
  for (int i = 0; avatar[i]; ++i) out[p++] = avatar[i];
  out[p++] = '\n';
  const char *k4 = "email:";
  for (int i = 0; k4[i]; ++i) out[p++] = k4[i];
  for (int i = 0; email[i]; ++i) out[p++] = email[i];
  out[p++] = '\n';
  const char *k5 = "password:";
  for (int i = 0; k5[i]; ++i) out[p++] = k5[i];
  for (int i = 0; pass[i]; ++i) out[p++] = pass[i];
  out[p++] = '\n';
  out[p] = 0;
  return pf_write(user, out, p);
}

static int seed_user_home(const char *user)
{
  char dir[128];
  const char *prefix = "/usr/home/";
  int p = 0;
  for (int i = 0; prefix[i] && p < (int)sizeof(dir) - 1; ++i) dir[p++] = prefix[i];
  for (int i = 0; user[i] && p < (int)sizeof(dir) - 1; ++i) dir[p++] = user[i];
  if (p >= (int)sizeof(dir) - 1) return -1;
  dir[p] = 0;

  if (brights_ramfs_mkdir(dir) < 0) {
    brights_ramfs_stat_t st;
    if (brights_ramfs_stat(dir, &st) < 0 || !st.is_dir) {
      return -1;
    }
  }

  char path[128];
  p = 0;
  for (int i = 0; dir[i] && p < (int)sizeof(path) - 1; ++i) path[p++] = dir[i];
  if (p >= (int)sizeof(path) - 12) return -1;
  path[p++] = '/';
  path[p++] = 'r';
  path[p++] = 'e';
  path[p++] = 'a';
  path[p++] = 'd';
  path[p++] = 'm';
  path[p++] = 'e';
  path[p++] = '.';
  path[p++] = 't';
  path[p++] = 'x';
  path[p++] = 't';
  path[p] = 0;

  int fd = brights_ramfs_open(path);
  if (fd < 0) fd = brights_ramfs_create(path);
  if (fd < 0) return -1;
  const char *msg = "Welcome to your home\n";
  return (brights_ramfs_write(fd, 0, msg, (uint64_t)strlen_s(msg)) >= 0) ? 0 : -1;
}

static int seed_user_config_dir(const char *user)
{
  char dir[128];
  const char *prefix = "/bin/config/";
  int p = 0;
  for (int i = 0; prefix[i] && p < (int)sizeof(dir) - 1; ++i) dir[p++] = prefix[i];
  for (int i = 0; user[i] && p < (int)sizeof(dir) - 1; ++i) dir[p++] = user[i];
  if (p >= (int)sizeof(dir) - 1) return -1;
  dir[p] = 0;

  if (brights_ramfs_mkdir(dir) < 0) {
    brights_ramfs_stat_t st;
    if (brights_ramfs_stat(dir, &st) < 0 || !st.is_dir) {
      return -1;
    }
  }
  return 0;
}

static void print_prompt(void)
{
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, current_user);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, is_root ? "# " : "$ ");
}

static void cmd_help(void)
{
  brights_serial_write_ascii(BRIGHTS_COM1_PORT,
    "commands: help echo pwd cd mkdir rmdir whoami login logout passwd useradd profile setpf ls stat cat touch write append cp mv rm hexdump bst kill jobs wifi ifconfig\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT,
    "bst: help procom\n");
}

static const char *proc_state_name(brights_proc_state_t state)
{
  switch (state) {
    case BRIGHTS_PROC_RUNNABLE:
      return "runnable";
    case BRIGHTS_PROC_RUNNING:
      return "running";
    case BRIGHTS_PROC_SLEEPING:
      return "sleeping";
    case BRIGHTS_PROC_UNUSED:
    default:
      return "unused";
  }
}

static void cmd_ls(const char *arg)
{
  char path[LIGHTSHELL_MAX_PATH];
  if (resolve_path(arg, path, sizeof(path)) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "invalid path\n");
    return;
  }
  brights_ramfs_stat_t st;
  if (brights_ramfs_stat(path, &st) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "not found\n");
    return;
  }
  if (!st.is_dir) {
    char base[LIGHTSHELL_MAX_PATH];
    if (path_basename(path, base, sizeof(base)) < 0) {
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, "invalid path\n");
      return;
    }
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, base);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  ");
    print_u64(st.size);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "B\n");
    return;
  }

  int shown = 0;
  for (int i = 0; i < BRIGHTS_RAMFS_MAX_FILES; ++i) {
    const char *name = brights_ramfs_name_at(i);
    if (!name || !is_direct_child(path, name)) {
      continue;
    }
    char base[LIGHTSHELL_MAX_PATH];
    if (path_basename(name, base, sizeof(base)) < 0) {
      continue;
    }
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, base);
    if (brights_ramfs_is_dir_fd(i)) {
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, "/\n");
    } else {
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  ");
      print_u64(brights_ramfs_size_at(i));
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, "B\n");
    }
    shown = 1;
  }
  if (!shown) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "(empty)\n");
  }
}

static void cmd_cat(const char *arg)
{
  arg = skip_spaces(arg);
  if (*arg == 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "usage: cat <name>\n");
    return;
  }
  char path[LIGHTSHELL_MAX_PATH];
  if (resolve_path(arg, path, sizeof(path)) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "invalid path\n");
    return;
  }
  int fd = brights_ramfs_open(path);
  if (fd < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "not found\n");
    return;
  }
  if (brights_ramfs_is_dir_fd(fd)) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "is a directory\n");
    return;
  }
  uint8_t buf[257];
  int64_t n = brights_ramfs_read(fd, 0, buf, 256);
  if (n <= 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "(empty)\n");
    return;
  }
  buf[n] = 0;
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, (const char *)buf);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
}

static void cmd_stat(const char *arg)
{
  arg = skip_spaces(arg);
  if (*arg == 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "usage: stat <name>\n");
    return;
  }
  char path[LIGHTSHELL_MAX_PATH];
  brights_ramfs_stat_t st;
  if (resolve_path(arg, path, sizeof(path)) < 0 || brights_ramfs_stat(path, &st) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "not found\n");
    return;
  }
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "path=");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, st.path);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, " type=");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, st.is_dir ? "dir" : "file");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, " size=");
  print_u64(st.size);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "B\n");
}

static void cmd_touch(const char *arg)
{
  arg = skip_spaces(arg);
  if (*arg == 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "usage: touch <name>\n");
    return;
  }
  char path[LIGHTSHELL_MAX_PATH];
  if (resolve_path(arg, path, sizeof(path)) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "invalid path\n");
    return;
  }
  int fd = brights_ramfs_open(path);
  if (fd >= 0) {
    if (brights_ramfs_is_dir_fd(fd)) {
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, "is a directory\n");
      return;
    }
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "ok\n");
    return;
  }
  if (brights_ramfs_create(path) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "create failed\n");
    return;
  }
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "ok\n");
}

static void cmd_write(const char *arg)
{
  arg = skip_spaces(arg);
  if (*arg == 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "usage: write <name> <text>\n");
    return;
  }

  char name[LIGHTSHELL_MAX_PATH];
  int ni = 0;
  while (*arg && *arg != ' ' && ni < (int)sizeof(name) - 1) {
    name[ni++] = *arg++;
  }
  name[ni] = 0;
  arg = skip_spaces(arg);

  if (name[0] == 0 || *arg == 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "usage: write <name> <text>\n");
    return;
  }

  char path[LIGHTSHELL_MAX_PATH];
  if (resolve_path(name, path, sizeof(path)) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "invalid path\n");
    return;
  }
  int fd = brights_ramfs_open(path);
  if (fd < 0) {
    fd = brights_ramfs_create(path);
  }
  if (fd < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "open/create failed\n");
    return;
  }
  if (brights_ramfs_is_dir_fd(fd)) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "is a directory\n");
    return;
  }
  int64_t n = brights_ramfs_write(fd, 0, arg, (uint64_t)strlen_s(arg));
  if (n < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "write failed\n");
    return;
  }
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "ok\n");
}

static void cmd_append(const char *arg)
{
  arg = skip_spaces(arg);
  if (*arg == 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "usage: append <name> <text>\n");
    return;
  }

  char name[LIGHTSHELL_MAX_PATH];
  int ni = 0;
  while (*arg && *arg != ' ' && ni < (int)sizeof(name) - 1) {
    name[ni++] = *arg++;
  }
  name[ni] = 0;
  arg = skip_spaces(arg);

  if (name[0] == 0 || *arg == 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "usage: append <name> <text>\n");
    return;
  }

  char path[LIGHTSHELL_MAX_PATH];
  if (resolve_path(name, path, sizeof(path)) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "invalid path\n");
    return;
  }
  int fd = brights_ramfs_open(path);
  if (fd < 0) {
    fd = brights_ramfs_create(path);
  }
  if (fd < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "open/create failed\n");
    return;
  }
  if (brights_ramfs_is_dir_fd(fd)) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "is a directory\n");
    return;
  }
  uint64_t off = brights_ramfs_file_size(fd);
  int64_t n = brights_ramfs_write(fd, off, arg, (uint64_t)strlen_s(arg));
  if (n < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "append failed\n");
    return;
  }
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "ok\n");
}

static void cmd_rm(const char *arg)
{
  arg = skip_spaces(arg);
  if (*arg == 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "usage: rm <name>\n");
    return;
  }
  char path[LIGHTSHELL_MAX_PATH];
  if (resolve_path(arg, path, sizeof(path)) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "invalid path\n");
    return;
  }
  if (brights_ramfs_unlink(path) == 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "ok\n");
  } else {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "remove failed\n");
  }
}

static void cmd_rmdir(const char *arg)
{
  arg = skip_spaces(arg);
  if (*arg == 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "usage: rmdir <path>\n");
    return;
  }
  char path[LIGHTSHELL_MAX_PATH];
  brights_ramfs_stat_t st;
  if (resolve_path(arg, path, sizeof(path)) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "invalid path\n");
    return;
  }
  if (brights_ramfs_stat(path, &st) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "not found\n");
    return;
  }
  if (!st.is_dir) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "not a directory\n");
    return;
  }
  if (brights_ramfs_unlink(path) == 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "ok\n");
  } else {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "rmdir failed\n");
  }
}

static int copy_file_path(const char *src_path, const char *dst_path)
{
  int src_fd = brights_ramfs_open(src_path);
  if (src_fd < 0 || brights_ramfs_is_dir_fd(src_fd)) {
    return -1;
  }

  int dst_fd = brights_ramfs_open(dst_path);
  if (dst_fd >= 0 && brights_ramfs_is_dir_fd(dst_fd)) {
    return -1;
  }
  if (dst_fd < 0) {
    dst_fd = brights_ramfs_create(dst_path);
  }
  if (dst_fd < 0) {
    return -1;
  }

  uint8_t buf[256];
  uint64_t off = 0;
  uint64_t total = brights_ramfs_file_size(src_fd);
  while (off < total) {
    int64_t n = brights_ramfs_read(src_fd, off, buf, sizeof(buf));
    if (n <= 0) {
      return -1;
    }
    int64_t w = brights_ramfs_write(dst_fd, off, buf, (uint64_t)n);
    if (w != n) {
      return -1;
    }
    off += (uint64_t)n;
  }
  return 0;
}

static void cmd_cp(const char *arg)
{
  char src_arg[LIGHTSHELL_MAX_PATH];
  char dst_arg[LIGHTSHELL_MAX_PATH];
  char src_path[LIGHTSHELL_MAX_PATH];
  char dst_path[LIGHTSHELL_MAX_PATH];
  brights_ramfs_stat_t src_st;

  if (parse_two_args(arg, src_arg, sizeof(src_arg), dst_arg, sizeof(dst_arg)) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "usage: cp <src> <dst>\n");
    return;
  }
  if (resolve_path(src_arg, src_path, sizeof(src_path)) < 0 ||
      resolve_path(dst_arg, dst_path, sizeof(dst_path)) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "invalid path\n");
    return;
  }
  if (streq(src_path, dst_path)) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "copy failed\n");
    return;
  }
  if (brights_ramfs_stat(src_path, &src_st) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "not found\n");
    return;
  }
  if (src_st.is_dir) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "is a directory\n");
    return;
  }
  if (copy_file_path(src_path, dst_path) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "copy failed\n");
    return;
  }
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "ok\n");
}

static void cmd_mv(const char *arg)
{
  char src_arg[LIGHTSHELL_MAX_PATH];
  char dst_arg[LIGHTSHELL_MAX_PATH];
  char src_path[LIGHTSHELL_MAX_PATH];
  char dst_path[LIGHTSHELL_MAX_PATH];
  brights_ramfs_stat_t src_st;

  if (parse_two_args(arg, src_arg, sizeof(src_arg), dst_arg, sizeof(dst_arg)) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "usage: mv <src> <dst>\n");
    return;
  }
  if (resolve_path(src_arg, src_path, sizeof(src_path)) < 0 ||
      resolve_path(dst_arg, dst_path, sizeof(dst_path)) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "invalid path\n");
    return;
  }
  if (streq(src_path, dst_path)) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "move failed\n");
    return;
  }
  if (brights_ramfs_stat(src_path, &src_st) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "not found\n");
    return;
  }
  if (src_st.is_dir) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "is a directory\n");
    return;
  }
  if (copy_file_path(src_path, dst_path) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "move failed\n");
    return;
  }
  if (brights_ramfs_unlink(src_path) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "move failed\n");
    return;
  }
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "ok\n");
}

static void cmd_hexdump(const char *arg)
{
  arg = skip_spaces(arg);
  if (*arg == 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "usage: hexdump <name>\n");
    return;
  }
  char path[LIGHTSHELL_MAX_PATH];
  if (resolve_path(arg, path, sizeof(path)) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "invalid path\n");
    return;
  }
  int fd = brights_ramfs_open(path);
  if (fd < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "not found\n");
    return;
  }
  if (brights_ramfs_is_dir_fd(fd)) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "is a directory\n");
    return;
  }

  uint8_t buf[128];
  int64_t n = brights_ramfs_read(fd, 0, buf, sizeof(buf));
  if (n <= 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "(empty)\n");
    return;
  }

  for (int64_t i = 0; i < n; ++i) {
    print_hex8(buf[i]);
    if ((i & 0xF) == 0xF) {
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
    } else {
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, " ");
    }
  }
  if ((n & 0xF) != 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
  }
}

static void cmd_mem(void)
{
  uint64_t used = 0;
  for (int i = 0; i < BRIGHTS_RAMFS_MAX_FILES; ++i) {
    used += brights_ramfs_size_at(i);
  }
  uint64_t cap = brights_ramfs_total_capacity();
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "BrightS Memory Information\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  Physical Total: ");
  print_u64(brights_pmem_total_bytes());
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, " bytes\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  Physical Free : ");
  print_u64(brights_pmem_free_bytes());
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, " bytes\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  RAMFS Used    : ");
  print_u64(used);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, " bytes\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  RAMFS Total   : ");
  print_u64(cap);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, " bytes\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  KMalloc Used  : ");
  print_u64((uint64_t)brights_kmalloc_used());
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, " bytes\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  KMalloc Total : ");
  print_u64((uint64_t)brights_kmalloc_capacity());
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, " bytes\n");
}

static void cmd_ps(void)
{
  uint32_t total = brights_proc_total();
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "BrightS Process Information\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  Total    : ");
  print_u64(total);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  Runnable : ");
  print_u64(brights_proc_count(BRIGHTS_PROC_RUNNABLE));
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  Running  : ");
  print_u64(brights_proc_count(BRIGHTS_PROC_RUNNING));
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  Sleeping : ");
  print_u64(brights_proc_count(BRIGHTS_PROC_SLEEPING));
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");

  for (uint32_t i = 0; i < 64u; ++i) {
    brights_proc_info_t info;
    if (brights_proc_info_at(i, &info) < 0) {
      continue;
    }
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  PID ");
    print_u64(info.pid);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, " : ");
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, proc_state_name(info.state));
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
  }
}

static void cmd_ticks(void)
{
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "BrightS Clock Information\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  Clock Ticks    : ");
  print_u64(brights_clock_now_ticks());
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  Sched Ticks    : ");
  print_u64(brights_sched_ticks());
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  Dispatch Count : ");
  print_u64(brights_sched_dispatches());
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
}

static void cmd_signal(void)
{
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "BrightS Signal Information\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  Pending Mask : 0x");
  uint32_t pending = brights_signal_pending();
  print_hex8((uint8_t)((pending >> 24) & 0xFFu));
  print_hex8((uint8_t)((pending >> 16) & 0xFFu));
  print_hex8((uint8_t)((pending >> 8) & 0xFFu));
  print_hex8((uint8_t)(pending & 0xFFu));
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
}

static int parse_u32(const char *s, uint32_t *out)
{
  uint32_t v = 0;
  int found = 0;
  while (*s >= '0' && *s <= '9') {
    found = 1;
    v = (v * 10u) + (uint32_t)(*s - '0');
    ++s;
  }
  if (!found || *s != 0 || !out) {
    return -1;
  }
  *out = v;
  return 0;
}

static void cmd_raise(const char *arg)
{
  arg = skip_spaces(arg);
  uint32_t signo = 0;
  if (parse_u32(arg, &signo) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "usage: raise <signo>\n");
    return;
  }
  if (brights_signal_raise(signo) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "raise failed\n");
    return;
  }
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "raise ok\n");
}

static void cmd_clearsig(const char *arg)
{
  arg = skip_spaces(arg);
  if (*arg == 0) {
    brights_signal_clear(brights_signal_global());
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "signals cleared\n");
    return;
  }

  uint32_t signo = 0;
  if (parse_u32(arg, &signo) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "usage: clearsig [signo]\n");
    return;
  }
  if (brights_signal_consume(brights_signal_global(), signo) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "signal not pending\n");
    return;
  }
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "signal cleared\n");
}
static void cmd_hooks(void)
{
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "BrightS Syscall Hook Information\n");

  /* Global statistics */
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  Active hooks : ");
  print_u64(brights_syshook_active_count());
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, " / ");
  print_u64(SYSHOOK_MAX);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");

  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  Total created: ");
  print_u64(brights_syshook_total_created());
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");

  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  Total events : ");
  print_u64(brights_syshook_total_events());
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n\n");

  /* List active hooks */
  int active_count = 0;
  for (int i = 0; i < SYSHOOK_MAX; ++i) {
    brights_hook_entry_t *entry = 0;
    if (brights_syshook_get_entry(i, &entry) != 0 || !entry) continue;
    if (!entry->active) continue;

    active_count++;
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  Hook[");
    print_u64((uint64_t)i);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "]: pid=");
    print_u64(entry->owner_pid);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, " pending=");
    print_u64(entry->count);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, " total=");
    print_u64(entry->total_events);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, " dropped=");
    print_u64(entry->dropped_events);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, " flags=");
    if (entry->flags & HOOK_FLAG_PRE) brights_serial_write_ascii(BRIGHTS_COM1_PORT, "PRE ");
    if (entry->flags & HOOK_FLAG_POST) brights_serial_write_ascii(BRIGHTS_COM1_PORT, "POST ");
    if (entry->flags & HOOK_FLAG_BROADCAST) brights_serial_write_ascii(BRIGHTS_COM1_PORT, "BCAST ");
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "watch=0x");
    print_hex8((uint8_t)((entry->watch_mask[0] >> 56) & 0xFF));
    print_hex8((uint8_t)((entry->watch_mask[0] >> 48) & 0xFF));
    print_hex8((uint8_t)((entry->watch_mask[0] >> 40) & 0xFF));
    print_hex8((uint8_t)((entry->watch_mask[0] >> 32) & 0xFF));
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
  }

  if (active_count == 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  (no active hooks)\n");
  }
}

static void cmd_hook_test(void)
{
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "=== Hook Subsystem Test ===\n\n");

  /* Show initial state */
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "Step 1: Initial state\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  Active hooks: ");
  print_u64(brights_syshook_active_count());
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");

  /* Register hook for sys_write (bit 2) with PRE and POST flags */
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\nStep 2: Register hook for sys_write (syscall 2)\n");
  uint64_t watch_lo = (1ULL << 2); /* syscall 2 = write */
  uint64_t watch_hi = 0;
  uint64_t flags = HOOK_FLAG_PRE | HOOK_FLAG_POST;

  int64_t hook_id = brights_sys_hook_register(watch_lo, watch_hi, flags);
  if (hook_id < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  FAILED: register returned ");
    print_u64((uint64_t)hook_id);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
    return;
  }
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  OK: hook_id=");
  print_u64((uint64_t)hook_id);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");

  /* Show state after registration */
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  Active hooks: ");
  print_u64(brights_syshook_active_count());
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");

  /* Trigger write syscalls to generate events */
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\nStep 3: Trigger 3 write syscalls\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  (this message triggers write syscall)\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  (this message triggers write syscall)\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  (this message triggers write syscall)\n");

  /* Poll for events */
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\nStep 4: Poll events (max 10)\n");
  brights_hook_event_t events[10];
  int64_t count = brights_sys_hook_poll((uint64_t)hook_id, (uint64_t)(uintptr_t)events, 10);

  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  Polled ");
  print_u64((uint64_t)count);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, " events\n");

  for (int i = 0; i < count && i < 10; ++i) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  [");
    print_u64((uint64_t)i);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "] type=");
    if (events[i].event_type == HOOK_EVT_PRE_SYSCALL) {
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, "PRE");
    } else if (events[i].event_type == HOOK_EVT_POST_SYSCALL) {
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, "POST");
    } else {
      print_u64(events[i].event_type);
    }
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, " syscall=");
    print_u64(events[i].syscall_nr);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, " pid=");
    print_u64(events[i].caller_pid);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, " ret=");
    print_u64((uint64_t)events[i].ret);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
  }

  /* Get hook info */
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\nStep 5: Hook statistics\n");
  brights_hook_entry_t *entry = 0;
  if (brights_syshook_get_entry((int)hook_id, &entry) == 0 && entry) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  total_events: ");
    print_u64(entry->total_events);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n  dropped_events: ");
    print_u64(entry->dropped_events);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n  pending: ");
    print_u64(entry->count);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
  }

  /* Unregister hook */
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\nStep 6: Unregister hook\n");
  if (brights_sys_hook_unregister((uint64_t)hook_id) == 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  OK: unregistered\n");
  } else {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  FAILED: unregister returned -1\n");
  }

  /* Final state */
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\nStep 7: Final state\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  Active hooks: ");
  print_u64(brights_syshook_active_count());
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n  Total created: ");
  print_u64(brights_syshook_total_created());
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n  Total events: ");
  print_u64(brights_syshook_total_events());
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n\n=== Test Complete ===\n");
}

static void cmd_bst_help(void)
{
  brights_serial_write_ascii(BRIGHTS_COM1_PORT,
    "BrightS BST Help\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT,
    "  Usage : bst <tool>\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT,
    "  Tools : help, procom\n");
}

static void cmd_bst_procom_help(void)
{
  brights_serial_write_ascii(BRIGHTS_COM1_PORT,
    "BrightS Procom Help\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT,
    "  Usage : bst procom <tool>\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT,
    "  Tools : version, cpu, acpi, memory, processes, clock, signals\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT,
    "          raise-signal, clear-signals, time, keyboard-test\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT,
    "          mount, clear, enter-user, reboot, shutdown\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT,
    "          hooks, hook-test\n");
}

static void cmd_cpuinfo(void)
{
  const brights_cpu_info_t *cpu = brights_hwinfo_cpu();
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "BrightS CPU Information\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  Vendor : ");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, cpu->vendor);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
  if (cpu->brand[0]) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  Brand  : ");
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, cpu->brand);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
  }
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  Family : ");
  print_u64(cpu->family);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, " Model: ");
  print_u64(cpu->model);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, " Stepping: ");
  print_u64(cpu->stepping);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  Cores  : ");
  print_u64(cpu->cores_per_pkg);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, " Threads: ");
  print_u64(cpu->logical_cores);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  L1d    : ");
  print_u64(cpu->l1d_size / 1024);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "KB  L1i: ");
  print_u64(cpu->l1i_size / 1024);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "KB\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  L2     : ");
  print_u64(cpu->l2_size / 1024);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "KB  L3: ");
  print_u64(cpu->l3_size / (1024 * 1024));
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "MB\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  TSC    : ");
  print_u64(cpu->tsc_freq / 1000000);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, " MHz");
  if (cpu->tsc_invariant) brights_serial_write_ascii(BRIGHTS_COM1_PORT, " (invariant)");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  Flags  :");
  if (cpu->has_sse42) brights_serial_write_ascii(BRIGHTS_COM1_PORT, " sse4.2");
  if (cpu->has_avx) brights_serial_write_ascii(BRIGHTS_COM1_PORT, " avx");
  if (cpu->has_avx2) brights_serial_write_ascii(BRIGHTS_COM1_PORT, " avx2");
  if (cpu->has_rdrand) brights_serial_write_ascii(BRIGHTS_COM1_PORT, " rdrand");
  if (cpu->has_x2apic) brights_serial_write_ascii(BRIGHTS_COM1_PORT, " x2apic");
  if (cpu->has_aes) brights_serial_write_ascii(BRIGHTS_COM1_PORT, " aes-ni");
  if (cpu->has_smep) brights_serial_write_ascii(BRIGHTS_COM1_PORT, " smep");
  if (cpu->has_smap) brights_serial_write_ascii(BRIGHTS_COM1_PORT, " smap");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
}

static void cmd_acpiinfo(void)
{
  const brights_acpi_info_t *acpi = brights_acpi_info();
  if (!acpi->ready) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "acpi not ready\n");
    return;
  }

  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "BrightS ACPI Information\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  OEM ID     : ");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, acpi->oem_id);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  OEM Table  : ");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, acpi->oem_table_id);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  Revision   : ");
  print_u64(acpi->revision);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  PM Profile : ");
  print_u64(acpi->pm_profile);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  SCI IRQ    : ");
  print_u64(acpi->sci_int);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  PM Timer   : ");
  print_u64(acpi->pm_tmr_blk);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  Reset Reg  : ");
  if (acpi->has_reset_reg) {
    print_u64(acpi->reset_reg_addr);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, " value=");
    print_u64(acpi->reset_value);
  } else {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "none");
  }
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
}

static int handle_bst_procom(const char *arg)
{
  arg = skip_spaces(arg);
  if (*arg == 0 || streq(arg, "help")) {
    cmd_bst_procom_help();
    return 1;
  }
  if (streq(arg, "version")) {
    const brights_cpu_info_t *cpu = brights_hwinfo_cpu();
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "BrightS System Information\n");
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  Kernel : BrightS x86_64\n");
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  Version: ");
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, version);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  Shell  : BrightS Lightshell\n");
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  CPU    : ");
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, cpu->vendor);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, " fam ");
    print_u64(cpu->family);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, " model ");
    print_u64(cpu->model);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
    return 1;
  }
  if (streq(arg, "cpu")) {
    cmd_cpuinfo();
    return 1;
  }
  if (streq(arg, "acpi")) {
    cmd_acpiinfo();
    return 1;
  }
  if (streq(arg, "memory")) {
    cmd_mem();
    return 1;
  }
  if (streq(arg, "processes")) {
    cmd_ps();
    return 1;
  }
  if (streq(arg, "clock")) {
    cmd_ticks();
    return 1;
  }
  if (streq(arg, "signals")) {
    cmd_signal();
    return 1;
  }
   if (streq(arg, "time")) {
     cmd_date();
     return 1;
   }
   if (streq(arg, "netget")) {
     return cmd_netget_handler("");
   }
   if (streq(arg, "keyboard-test")) {
    cmd_kbdtest();
    return 1;
  }
  if (streq(arg, "mount")) {
    cmd_mount();
    return 1;
  }
  if (streq(arg, "clear")) {
    cmd_clear();
    return 1;
  }
  if (streq(arg, "enter-user")) {
    return cmd_runuser();
  }
  if (streq(arg, "reboot")) {
    return cmd_reboot();
  }
  if (streq(arg, "shutdown")) {
    return cmd_halt();
  }
  if (streq(arg, "hooks")) {
    cmd_hooks();
    return 1;
  }
  if (streq(arg, "hook-test")) {
    cmd_hook_test();
    return 1;
  }
  if (starts_with(arg, "raise-signal ")) {
    cmd_raise(arg + 13);
    return 1;
  }
  if (streq(arg, "clear-signals") || starts_with(arg, "clear-signals ")) {
    cmd_clearsig(arg + 13);
    return 1;
  }
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "unknown bst procom tool\n");
  return 1;
}

static void cmd_uname(void)
{
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "BrightS x86_64\n");
}

static void cmd_mount(void)
{
  if (brights_vfs_mount_external("manual") == 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "mounted at /mnt/drive/\n");
  } else {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "mount failed\n");
  }
}

static void cmd_clear(void)
{
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\x1b[2J\x1b[H");
}

static int cmd_runuser(void)
{
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "entering user mode\n");
  brights_userinit_enter();
  return 1;
}

static int cmd_reboot(void)
{
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "rebooting\n");
  outb(0x64, 0xFE);
  for (;;) {
    __asm__ __volatile__("hlt");
  }
  return 0;
}

static int cmd_halt(void)
{
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "system halted\n");
  for (;;) {
    __asm__ __volatile__("hlt");
  }
  return 0;
}

static int handle_bst(const char *arg)
{
  arg = skip_spaces(arg);
  if (*arg == 0 || streq(arg, "help")) {
    cmd_bst_help();
    return 1;
  }
  if (streq(arg, "procom")) {
    return handle_bst_procom("");
  }
  if (starts_with(arg, "procom ")) {
    return handle_bst_procom(arg + 7);
  }

  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "unknown bst tool\n");
  return 1;
}

static void cmd_echo(const char *arg)
{
  arg = skip_spaces(arg);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, arg);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
}

static void cmd_kill(const char *arg)
{
  arg = skip_spaces(arg);
  if (*arg == 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "usage: kill <pid>\n");
    return;
  }

  uint32_t pid = 0;
  int found = 0;
  while (*arg >= '0' && *arg <= '9') {
    found = 1;
    pid = pid * 10 + (uint32_t)(*arg - '0');
    ++arg;
  }
  if (!found || *arg != 0 || pid == 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "invalid pid\n");
    return;
  }

  /* Send SIGTERM (signal 15) to the process */
  brights_proc_info_t info;
  if (brights_proc_get_by_pid(pid, &info) != 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "no such process\n");
    return;
  }

  brights_signal_raise(15);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "killed pid ");
  print_u64(pid);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
}

static void cmd_jobs(void)
{
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "BrightS Job Information\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  PID   STATE      NAME\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "----- ---------- --------------------------------\n");

  for (uint32_t i = 0; i < 64; ++i) {
    brights_proc_info_t info;
    if (brights_proc_info_at(i, &info) < 0) continue;
    if (info.state == BRIGHTS_PROC_UNUSED) continue;

    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  PID ");
    print_u64(info.pid);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, " : ");
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, proc_state_name(info.state));
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "      ");
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, info.name);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
  }
}

static void cmd_date(void)
{
  brights_rtc_time_t t;
  if (brights_rtc_read(&t) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "date read failed\n");
    return;
  }
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "BrightS Time Information\n");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  RTC Time : ");
  print_u4(t.year);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "-");
  print_u2(t.month);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "-");
  print_u2(t.day);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, " ");
  print_u2(t.hour);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, ":");
  print_u2(t.minute);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, ":");
  print_u2(t.second);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
}

static void cmd_kbdtest(void)
{
  int old_mode = brights_tty_get_mode();
  brights_tty_set_mode(TTY_MODE_RAW);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "kbdtest: type keys (ESC to exit)\n");
  for (;;) {
    char ch = 0;
    int r = brights_tty_read_char(&ch);
    if (r <= 0) {
      continue;
    }
    if (ch == 27) {
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\nkbdtest: done\n");
      brights_tty_set_mode(old_mode);
      return;
    }
    char out[2] = {ch, 0};
    if (ch == '\n') {
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
    } else if (ch == '\b') {
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\b \b");
    } else {
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, out);
    }
  }
}

static void cmd_whoami(void)
{
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, current_user);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
}

static void cmd_profile(void)
{
  if (pf_show(current_user) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "profile not found\n");
  }
}

static void cmd_setpf(const char *arg)
{
  arg = skip_spaces(arg);
  const char *sp = find_space(arg);
  if (*arg == 0 || *sp == 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "usage: setpf <hostname|avatar|email> <value>\n");
    return;
  }
  char key[32];
  int klen = (int)(sp - arg);
  if (klen <= 0 || klen >= (int)sizeof(key)) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "invalid key\n");
    return;
  }
  for (int i = 0; i < klen; ++i) key[i] = arg[i];
  key[klen] = 0;
  const char *val = skip_spaces(sp);
  if (*val == 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "invalid value\n");
    return;
  }
  if (pf_set_field(current_user, key, val) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "setpf failed\n");
    return;
  }
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "setpf ok\n");
}

static void cmd_login(const char *arg)
{
  arg = skip_spaces(arg);
  const char *sp = find_space(arg);
  if (*arg == 0 || *sp == 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "usage: login <user> <pass>\n");
    return;
  }

  char user[LIGHTSHELL_MAX_USER];
  int ulen = (int)(sp - arg);
  if (ulen <= 0 || ulen >= LIGHTSHELL_MAX_USER) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "invalid user\n");
    return;
  }
  for (int i = 0; i < ulen; ++i) user[i] = arg[i];
  user[ulen] = 0;

  const char *pass = skip_spaces(sp);
  if (*pass == 0 || strlen_s(pass) >= LIGHTSHELL_MAX_PASS) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "invalid pass\n");
    return;
  }

  char expected[LIGHTSHELL_MAX_PASS];
  if (pf_get_password(user, expected, sizeof(expected)) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "login failed\n");
    return;
  }
  if (!streq(expected, pass)) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "login failed\n");
    return;
  }
  str_copy(current_user, sizeof(current_user), user);
  is_root = streq(current_user, "root");
  if (is_root) {
    str_copy(current_dir, sizeof(current_dir), "/usr/home/root");
  } else {
    char home[LIGHTSHELL_MAX_PATH] = "/usr/home/";
    int p = strlen_s(home);
    for (int i = 0; user[i] && p < (int)sizeof(home) - 1; ++i) {
      home[p++] = user[i];
    }
    home[p] = 0;
    str_copy(current_dir, sizeof(current_dir), home);
  }
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "login ok\n");
}

static void cmd_logout(void)
{
  str_copy(current_user, sizeof(current_user), "guest");
  str_copy(current_dir, sizeof(current_dir), "/");
  is_root = 0;
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "logout ok\n");
}

static void cmd_passwd(const char *arg)
{
  arg = skip_spaces(arg);
  if (*arg == 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "usage: passwd <newpass>\n");
    return;
  }
  if (pf_set_password(current_user, arg) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "passwd failed\n");
    return;
  }
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "passwd ok\n");
}

static void cmd_useradd(const char *arg)
{
  if (!is_root) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "permission denied\n");
    return;
  }
  arg = skip_spaces(arg);
  const char *sp = find_space(arg);
  if (*arg == 0 || *sp == 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "usage: useradd <user> <pass>\n");
    return;
  }
  char user[LIGHTSHELL_MAX_USER];
  int ulen = (int)(sp - arg);
  if (ulen <= 0 || ulen >= LIGHTSHELL_MAX_USER) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "invalid user\n");
    return;
  }
  for (int i = 0; i < ulen; ++i) user[i] = arg[i];
  user[ulen] = 0;
  const char *pass = skip_spaces(sp);
  if (*pass == 0 || strlen_s(pass) >= LIGHTSHELL_MAX_PASS) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "invalid pass\n");
    return;
  }
  if (pf_exists(user) == 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "user exists\n");
    return;
  }
  if (seed_user_config_dir(user) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "useradd failed\n");
    return;
  }
  if (pf_write_default(user, pass) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "useradd failed\n");
    return;
  }
  seed_user_home(user);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "useradd ok\n");
}

static void cmd_wifi(const char *arg)
{
  arg = skip_spaces(arg);
  if (*arg == 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "usage: wifi <command> [args]\n");
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  commands: scan, connect <ssid> [pass], disconnect, status, list, up, down\n");
    return;
  }

  if (streq(arg, "scan")) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "wifi: scanning...\n");
    brights_wifi_scan("wlan0");
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "wifi: scan complete\n");
    return;
  }

  if (streq(arg, "up")) {
    brights_wifi_if_up("wlan0");
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "wifi: wlan0 up\n");
    return;
  }

  if (streq(arg, "down")) {
    brights_wifi_if_down("wlan0");
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "wifi: wlan0 down\n");
    return;
  }

  if (streq(arg, "status")) {
    brights_wifi_if_t *iface = brights_wifi_if_get("wlan0");
    if (!iface) {
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, "wifi: no interface\n");
      return;
    }
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "wifi: wlan0 state=");
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, brights_wifi_state_name(iface->state));
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, " ssid=");
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, iface->ssid);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, " channel=");
    print_u64(iface->channel);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, " rssi=");
    print_u64((uint64_t)(iface->rssi + 256));
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, " dBm\n");
    return;
  }

  if (streq(arg, "list")) {
    brights_wifi_if_t *iface = brights_wifi_if_get("wlan0");
    if (!iface) {
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, "wifi: no interface\n");
      return;
    }
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "wifi: available networks\n");
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  BSSID            SSID                           CH  RSSI  SEC\n");
    for (int i = 0; i < iface->bss_count; ++i) {
      brights_wifi_bss_t *bss = &iface->bss_list[i];
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  ");
      for (int j = 0; j < 6; ++j) {
        print_hex8(bss->bssid[j]);
        if (j < 5) brights_serial_write_ascii(BRIGHTS_COM1_PORT, ":");
      }
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, " ");
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, bss->ssid);
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  ");
      print_u64(bss->channel);
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  ");
      print_u64((uint64_t)(bss->rssi + 256));
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  ");
      if (bss->rsn) brights_serial_write_ascii(BRIGHTS_COM1_PORT, "WPA2");
      else if (bss->wpa) brights_serial_write_ascii(BRIGHTS_COM1_PORT, "WPA");
      else brights_serial_write_ascii(BRIGHTS_COM1_PORT, "OPEN");
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
    }
    return;
  }

  if (starts_with(arg, "connect ")) {
    arg += 8;
    const char *sp = find_space(arg);
    if (*arg == 0) {
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, "usage: wifi connect <ssid> [password]\n");
      return;
    }
    char ssid[64];
    int slen = 0;
    if (*sp == 0) {
      while (*arg && slen < 63) ssid[slen++] = *arg++;
    } else {
      while (arg < sp && slen < 63) ssid[slen++] = *arg++;
    }
    ssid[slen] = 0;

    const char *pass = 0;
    if (*sp != 0) {
      pass = skip_spaces(sp);
    }

    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "wifi: connecting to ");
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, ssid);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "...\n");

    if (brights_wifi_connect("wlan0", ssid, pass) < 0) {
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, "wifi: connect failed\n");
    }
    return;
  }

  if (streq(arg, "disconnect")) {
    brights_wifi_disconnect("wlan0");
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "wifi: disconnected\n");
    return;
  }

  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "wifi: unknown command\n");
}

static void cmd_ifconfig(const char *arg)
{
  arg = skip_spaces(arg);
  if (*arg == 0) {
    /* Print all interfaces */
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "Interfaces:\n");

    /* Ethernet */
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  eth0: ");
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "192.168.1.100/24 gw=192.168.1.1 UP\n");

    /* WiFi */
    brights_wifi_if_t *wlan = brights_wifi_if_get("wlan0");
    if (wlan && wlan->up) {
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  wlan0: state=");
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, brights_wifi_state_name(wlan->state));
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, " ssid=");
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, wlan->ssid);
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, " rssi=");
      print_u64((uint64_t)(wlan->rssi + 256));
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, " dBm\n");
    } else {
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  wlan0: DOWN\n");
    }
    return;
  }

  if (streq(arg, "init")) {
    brights_net_init();
    uint8_t mac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    brights_netif_add("eth0", mac);
    brights_netif_set_ip("eth0", 0xC0A80164, 0xFFFFFF00, 0xC0A80101);
    brights_netif_up("eth0");

    uint8_t wlan_mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    brights_wifi_if_add("wlan0", wlan_mac);
    brights_wifi_if_up("wlan0");

    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "ifconfig: interfaces initialized\n");
    return;
  }

  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "usage: ifconfig [init]\n");
}

static void cmd_cd(const char *arg)
{
  arg = skip_spaces(arg);
  if (*arg == 0) {
    if (is_root) {
      str_copy(current_dir, sizeof(current_dir), "/usr/home/root");
    } else {
      str_copy(current_dir, sizeof(current_dir), "/usr/home/guest");
    }
    return;
  }
  char path[LIGHTSHELL_MAX_PATH];
  brights_ramfs_stat_t st;
  if (resolve_path(arg, path, sizeof(path)) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "invalid path\n");
    return;
  }
  if (brights_ramfs_stat(path, &st) < 0 || !st.is_dir) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "not a directory\n");
    return;
  }
  str_copy(current_dir, sizeof(current_dir), path);
}

static void cmd_mkdir(const char *arg)
{
  char path[LIGHTSHELL_MAX_PATH];
  arg = skip_spaces(arg);
  if (*arg == 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "usage: mkdir <path>\n");
    return;
  }
  if (resolve_path(arg, path, sizeof(path)) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "invalid path\n");
    return;
  }
  if (brights_ramfs_mkdir(path) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "mkdir failed\n");
    return;
  }
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "ok\n");
}

static int handle_line(char *line)
{
  const char *cmd = skip_spaces(line);
  if (*cmd == 0) return 1;

  /* Extract command name (up to first space) */
  char cmd_name[32];
  int i = 0;
  while (cmd[i] && cmd[i] != ' ' && i < 31) {
    cmd_name[i] = cmd[i];
    ++i;
  }
  cmd_name[i] = 0;

  /* Binary search lookup - O(log n) */
  const cmd_entry_t *entry = cmd_find(cmd_name);
  if (entry) {
    const char *arg = skip_spaces(cmd + i);
    return entry->handler(arg);
  }

  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "unknown command\n");
  return 1;
}

/* ===== Handler wrappers (bridge old void functions to int (*)(const char *)) ===== */

static int cmd_help_handler(const char *arg)
{
  (void)arg;
  cmd_help();
  return 1;
}

static int cmd_ls_handler(const char *arg)
{
  cmd_ls(arg);
  return 1;
}

static int cmd_pwd_handler(const char *arg)
{
  (void)arg;
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, current_dir);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
  return 1;
}

static int cmd_cd_handler(const char *arg)
{
  cmd_cd(arg);
  return 1;
}

static int cmd_mkdir_handler(const char *arg)
{
  cmd_mkdir(arg);
  return 1;
}

static int cmd_rmdir_handler(const char *arg)
{
  cmd_rmdir(arg);
  return 1;
}

static int cmd_cat_handler(const char *arg)
{
  cmd_cat(arg);
  return 1;
}

static int cmd_stat_handler(const char *arg)
{
  cmd_stat(arg);
  return 1;
}

static int cmd_rm_handler(const char *arg)
{
  cmd_rm(arg);
  return 1;
}

static int cmd_cp_handler(const char *arg)
{
  cmd_cp(arg);
  return 1;
}

static int cmd_mv_handler(const char *arg)
{
  cmd_mv(arg);
  return 1;
}

static int cmd_echo_handler(const char *arg)
{
  cmd_echo(arg);
  return 1;
}

static int cmd_kill_handler(const char *arg)
{
  cmd_kill(arg);
  return 1;
}

static int cmd_jobs_handler(const char *arg)
{
  (void)arg;
  cmd_jobs();
  return 1;
}

static int cmd_wifi_handler(const char *arg)
{
  cmd_wifi(arg);
  return 1;
}

static int cmd_ifconfig_handler(const char *arg)
{
  cmd_ifconfig(arg);
  return 1;
}

static int cmd_bst_handler(const char *arg)
{
  return handle_bst(arg);
}

static int cmd_login_handler(const char *arg)
{
  cmd_login(arg);
  return 1;
}

static int cmd_logout_handler(const char *arg)
{
  (void)arg;
  cmd_logout();
  return 1;
}

static int cmd_whoami_handler(const char *arg)
{
  (void)arg;
  cmd_whoami();
  return 1;
}

static int cmd_profile_handler(const char *arg)
{
  (void)arg;
  cmd_profile();
  return 1;
}

static int cmd_passwd_handler(const char *arg)
{
  cmd_passwd(arg);
  return 1;
}

static int cmd_useradd_handler(const char *arg)
{
  cmd_useradd(arg);
  return 1;
}

static int cmd_setpf_handler(const char *arg)
{
  cmd_setpf(arg);
  return 1;
}

static int cmd_touch_handler(const char *arg)
{
  cmd_touch(arg);
  return 1;
}

static int cmd_write_handler(const char *arg)
{
  cmd_write(arg);
  return 1;
}

static int cmd_append_handler(const char *arg)
{
  cmd_append(arg);
  return 1;
}

static int cmd_hexdump_handler(const char *arg)
{
  cmd_hexdump(arg);
  return 1;
}

static int cmd_uname_handler(const char *arg)
{
  (void)arg;
  cmd_uname();
  return 1;
}

static int cmd_mount_handler(const char *arg)
{
  (void)arg;
  cmd_mount();
  return 1;
}

static int cmd_clear_handler(const char *arg)
{
  (void)arg;
  cmd_clear();
  return 1;
}

static int cmd_reboot_handler(const char *arg)
{
  (void)arg;
  return cmd_reboot();
}

static int cmd_halt_handler(const char *arg)
{
  (void)arg;
  return cmd_halt();
}

static int cmd_date_handler(const char *arg)
{
  (void)arg;
  cmd_date();
  return 1;
}

#ifdef BRIGHTS_LIGHTSHELL_TESTING
void brights_lightshell_reset_for_test(void)
{
  str_copy(current_user, sizeof(current_user), "guest");
  str_copy(current_dir, sizeof(current_dir), "/");
  is_root = 0;
}

const char *brights_lightshell_current_dir(void)
{
  return current_dir;
}

void brights_lightshell_set_current_dir(const char *path)
{
  kutil_strncpy(current_dir, path, sizeof(current_dir) - 1);
  current_dir[sizeof(current_dir) - 1] = '\0';
}

int brights_lightshell_eval_for_test(char *line)
{
  return handle_line(line);
}

const char *brights_lightshell_current_user_for_test(void)
{
  return current_user;
}

const char *brights_lightshell_current_dir_for_test(void)
{
  return current_dir;
}

int brights_lightshell_is_root_for_test(void)
{
  return is_root;
}
#endif

static void history_add(const char *line)
{
  if (!line || line[0] == 0) {
    return;
  }
  
  // Don't add duplicate of last command
  if (history_count > 0 && streq(history[history_count - 1], line)) {
    return;
  }
  
  if (history_count < LIGHTSHELL_HISTORY_SIZE) {
    str_copy(history[history_count], LIGHTSHELL_MAX_LINE, line);
    ++history_count;
  } else {
    // Shift history up
    for (int i = 0; i < LIGHTSHELL_HISTORY_SIZE - 1; ++i) {
      str_copy(history[i], LIGHTSHELL_MAX_LINE, history[i + 1]);
    }
    str_copy(history[LIGHTSHELL_HISTORY_SIZE - 1], LIGHTSHELL_MAX_LINE, line);
  }
  history_index = history_count;
  history_nav_index = history_count;
}

static void clear_line(int len)
{
  for (int i = 0; i < len; ++i) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\b \b");
  }
}

static void redraw_line(char *line, int *len)
{
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, line);
  *len = strlen_s(line);
}

static int tab_complete(char *line, int *len)
{
  if (*len == 0) {
    return 0;
  }
  
  // Find matches
  int match_count = 0;
  const char *match = 0;
  int match_len = 0;
  
  for (int i = 0; commands[i]; ++i) {
    if (starts_with(commands[i], line)) {
      if (match_count == 0) {
        match = commands[i];
        match_len = strlen_s(commands[i]);
      }
      ++match_count;
    }
  }
  
  if (match_count == 0) {
    return 0;
  }
  
  if (match_count == 1) {
    // Single match - complete it
    clear_line(*len);
    str_copy(line, LIGHTSHELL_MAX_LINE, match);
    line[match_len] = ' ';
    line[match_len + 1] = 0;
    *len = match_len + 1;
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, line);
    return 1;
  }
  
  // Multiple matches - find common prefix
  int common_len = 0;
  char first[LIGHTSHELL_MAX_LINE];
  str_copy(first, LIGHTSHELL_MAX_LINE, match);
  
  for (int i = 0; commands[i]; ++i) {
    if (starts_with(commands[i], line)) {
      if (common_len == 0) {
        common_len = strlen_s(commands[i]);
      } else {
        int j = 0;
        while (first[j] && commands[i][j] && first[j] == commands[i][j]) {
          ++j;
        }
        common_len = j;
      }
    }
  }
  
  if (common_len > *len) {
    clear_line(*len);
    for (int i = 0; i < common_len; ++i) {
      line[i] = first[i];
    }
    line[common_len] = 0;
    *len = common_len;
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, line);
  } else {
    // Show all matches
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
    for (int i = 0; commands[i]; ++i) {
      if (starts_with(commands[i], line)) {
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, commands[i]);
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, " ");
      }
    }
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
    print_prompt();
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, line);
  }
  
  return 1;
}

void brights_lightshell_run(void)
{
  char line[LIGHTSHELL_MAX_LINE];
  int len = 0;
  int escape_state = 0;

  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "brights shell ready\n");
  cmd_help();
  print_prompt();

  for (;;) {
    uint8_t ch = (uint8_t)brights_tty_read_char_blocking();

    // Handle escape sequences for arrow keys
    if (escape_state == 1) {
      if (ch == '[') {
        escape_state = 2;
        continue;
      }
      escape_state = 0;
    }
    
    if (escape_state == 2) {
      escape_state = 0;
      if (ch == 'A' && history_count > 0) {
        // Up arrow - previous command
        if (history_nav_index > 0) {
          --history_nav_index;
          clear_line(len);
          str_copy(line, LIGHTSHELL_MAX_LINE, history[history_nav_index]);
          redraw_line(line, &len);
        }
        continue;
      } else if (ch == 'B' && history_count > 0) {
        // Down arrow - next command
        if (history_nav_index < history_count) {
          ++history_nav_index;
          clear_line(len);
          if (history_nav_index < history_count) {
            str_copy(line, LIGHTSHELL_MAX_LINE, history[history_nav_index]);
          } else {
            line[0] = 0;
          }
          redraw_line(line, &len);
        }
        continue;
      }
    }

    if (ch == 0x1B) { // ESC
      escape_state = 1;
      continue;
    }

    if (ch == '\r' || ch == '\n') {
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
      line[len] = 0;
      if (len > 0) {
        history_add(line);
      }
      if (!handle_line(line)) {
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "bye\n");
        return;
      }
      len = 0;
      escape_state = 0;
      print_prompt();
      continue;
    }

    if ((ch == 0x08 || ch == 0x7F) && len > 0) {
      --len;
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\b \b");
      continue;
    }

    if (ch == 0x09) { // Tab
      line[len] = 0;
      tab_complete(line, &len);
      continue;
    }

    if (ch >= 32 && ch < 127 && len < LIGHTSHELL_MAX_LINE - 1) {
      line[len++] = (char)ch;
      char echo[2] = {(char)ch, 0};
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, echo);
    }
  }
  cmd_clear();
}
