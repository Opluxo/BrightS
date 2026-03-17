#include "kshell.h"
#include "../dev/serial.h"
#include "../platform/x86_64/io.h"
#include "../fs/ramfs.h"
#include "../fs/btrfs.h"
#include "../fs/vfs.h"
#include "../dev/rtc.h"
#include "../dev/ps2kbd.h"
#include "userinit.h"
#include <stdint.h>

#define KSHELL_MAX_LINE 256
#define KSHELL_MAX_USER 32
#define KSHELL_MAX_PASS 64
#define KSHELL_MAX_CFG  1024

static char current_user[KSHELL_MAX_USER] = "guest";
static int is_root = 0;

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

static int userpf_path(const char *user, char *out, int cap)
{
  const char *prefix = "/config/";
  const char *suffix = "/example.pf";
  int p = 0;
  int ulen = strlen_s(user);
  if (ulen <= 0 || ulen >= KSHELL_MAX_USER) {
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
  char content[KSHELL_MAX_CFG];
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
  char cfg[KSHELL_MAX_CFG];
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
  char cfg[KSHELL_MAX_CFG];
  if (pf_read(user, cfg, sizeof(cfg)) < 0) {
    return -1;
  }
  char uname[KSHELL_MAX_USER];
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

  char out[KSHELL_MAX_CFG];
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
  char cfg[KSHELL_MAX_CFG];
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
  char cfg[KSHELL_MAX_CFG];
  if (pf_read(user, cfg, sizeof(cfg)) < 0) {
    return -1;
  }
  char uname[KSHELL_MAX_USER];
  char host[64];
  char avatar[64];
  char email[128];
  char pass[KSHELL_MAX_PASS];
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

  char out[KSHELL_MAX_CFG];
  int p = 0;
  const char *k1 = "username:"; for (int i = 0; k1[i]; ++i) out[p++] = k1[i];
  for (int i = 0; uname[i]; ++i) out[p++] = uname[i]; out[p++] = '\n';
  const char *k2 = "hostname:"; for (int i = 0; k2[i]; ++i) out[p++] = k2[i];
  for (int i = 0; host[i]; ++i) out[p++] = host[i]; out[p++] = '\n';
  const char *k3 = "avatar:"; for (int i = 0; k3[i]; ++i) out[p++] = k3[i];
  for (int i = 0; avatar[i]; ++i) out[p++] = avatar[i]; out[p++] = '\n';
  const char *k4 = "email:"; for (int i = 0; k4[i]; ++i) out[p++] = k4[i];
  for (int i = 0; email[i]; ++i) out[p++] = email[i]; out[p++] = '\n';
  const char *k5 = "password:"; for (int i = 0; k5[i]; ++i) out[p++] = k5[i];
  for (int i = 0; pass[i]; ++i) out[p++] = pass[i]; out[p++] = '\n';
  out[p] = 0;
  return pf_write(user, out, p);
}

static int seed_user_home(const char *user)
{
  char path[128];
  const char *prefix = "/usr/home/";
  const char *suffix = "/readme.txt";
  int p = 0;
  for (int i = 0; prefix[i] && p < (int)sizeof(path) - 1; ++i) path[p++] = prefix[i];
  for (int i = 0; user[i] && p < (int)sizeof(path) - 1; ++i) path[p++] = user[i];
  for (int i = 0; suffix[i] && p < (int)sizeof(path) - 1; ++i) path[p++] = suffix[i];
  if (p >= (int)sizeof(path) - 1) return -1;
  path[p] = 0;

  int fd = brights_ramfs_open(path);
  if (fd < 0) fd = brights_ramfs_create(path);
  if (fd < 0) return -1;
  const char *msg = "Welcome to your home\n";
  return (brights_ramfs_write(fd, 0, msg, (uint64_t)strlen_s(msg)) >= 0) ? 0 : -1;
}

static void print_prompt(void)
{
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, current_user);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, is_root ? "# " : "$ ");
}

static void cmd_help(void)
{
  brights_serial_write_ascii(BRIGHTS_COM1_PORT,
    "commands: help echo uname pwd whoami login logout passwd useradd profile setpf ls stat cat touch write append rm hexdump mem date kbdtest mount runuser clear reboot halt\n");
}

static void cmd_ls(void)
{
  int count = brights_ramfs_count();
  if (count == 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "(empty)\n");
    return;
  }
  for (int i = 0; i < BRIGHTS_RAMFS_MAX_FILES; ++i) {
    const char *name = brights_ramfs_name_at(i);
    if (!name) {
      continue;
    }
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, name);
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "  ");
    print_u64(brights_ramfs_size_at(i));
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "B\n");
  }
}

static void cmd_cat(const char *arg)
{
  arg = skip_spaces(arg);
  if (*arg == 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "usage: cat <name>\n");
    return;
  }
  int fd = brights_ramfs_open(arg);
  if (fd < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "not found\n");
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
  int fd = brights_ramfs_open(arg);
  if (fd < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "not found\n");
    return;
  }
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "name=");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, arg);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, " size=");
  print_u64(brights_ramfs_file_size(fd));
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "B\n");
}

static void cmd_touch(const char *arg)
{
  arg = skip_spaces(arg);
  if (*arg == 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "usage: touch <name>\n");
    return;
  }
  if (brights_ramfs_create(arg) < 0) {
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

  char name[64];
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

  int fd = brights_ramfs_open(name);
  if (fd < 0) {
    fd = brights_ramfs_create(name);
  }
  if (fd < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "open/create failed\n");
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

  char name[64];
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

  int fd = brights_ramfs_open(name);
  if (fd < 0) {
    fd = brights_ramfs_create(name);
  }
  if (fd < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "open/create failed\n");
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
  if (brights_ramfs_unlink(arg) == 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "ok\n");
  } else {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "not found\n");
  }
}

static void cmd_hexdump(const char *arg)
{
  arg = skip_spaces(arg);
  if (*arg == 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "usage: hexdump <name>\n");
    return;
  }
  int fd = brights_ramfs_open(arg);
  if (fd < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "not found\n");
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
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "ramfs used=");
  print_u64(used);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, " bytes total=");
  print_u64(cap);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, " bytes\n");
}

static void cmd_echo(const char *arg)
{
  arg = skip_spaces(arg);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, arg);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
}

static void cmd_date(void)
{
  brights_rtc_time_t t;
  if (brights_rtc_read(&t) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "date read failed\n");
    return;
  }
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
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "kbdtest: type keys (ESC to exit)\n");
  for (;;) {
    char ch = 0;
    int r = brights_ps2kbd_read_char(&ch);
    if (r <= 0) {
      continue;
    }
    if (ch == 27) {
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\nkbdtest: done\n");
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

  char user[KSHELL_MAX_USER];
  int ulen = (int)(sp - arg);
  if (ulen <= 0 || ulen >= KSHELL_MAX_USER) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "invalid user\n");
    return;
  }
  for (int i = 0; i < ulen; ++i) user[i] = arg[i];
  user[ulen] = 0;

  const char *pass = skip_spaces(sp);
  if (*pass == 0 || strlen_s(pass) >= KSHELL_MAX_PASS) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "invalid pass\n");
    return;
  }

  char expected[KSHELL_MAX_PASS];
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
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "login ok\n");
}

static void cmd_logout(void)
{
  str_copy(current_user, sizeof(current_user), "guest");
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
  char user[KSHELL_MAX_USER];
  int ulen = (int)(sp - arg);
  if (ulen <= 0 || ulen >= KSHELL_MAX_USER) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "invalid user\n");
    return;
  }
  for (int i = 0; i < ulen; ++i) user[i] = arg[i];
  user[ulen] = 0;
  const char *pass = skip_spaces(sp);
  if (*pass == 0 || strlen_s(pass) >= KSHELL_MAX_PASS) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "invalid pass\n");
    return;
  }
  if (pf_exists(user) == 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "user exists\n");
    return;
  }
  if (pf_write_default(user, pass) < 0) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "useradd failed\n");
    return;
  }
  seed_user_home(user);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "useradd ok\n");
}

static int handle_line(char *line)
{
  const char *cmd = skip_spaces(line);
  if (*cmd == 0) {
    return 1;
  }
  if (streq(cmd, "help")) {
    cmd_help();
    return 1;
  }
  if (streq(cmd, "ls")) {
    cmd_ls();
    return 1;
  }
  if (streq(cmd, "uname")) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "BrightS x86_64\n");
    return 1;
  }
  if (streq(cmd, "pwd")) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "/\n");
    return 1;
  }
  if (streq(cmd, "whoami")) {
    cmd_whoami();
    return 1;
  }
  if (streq(cmd, "profile")) {
    cmd_profile();
    return 1;
  }
  if (streq(cmd, "logout")) {
    cmd_logout();
    return 1;
  }
  if (streq(cmd, "mem")) {
    cmd_mem();
    return 1;
  }
  if (streq(cmd, "date")) {
    cmd_date();
    return 1;
  }
  if (streq(cmd, "kbdtest")) {
    cmd_kbdtest();
    return 1;
  }
  if (streq(cmd, "mount")) {
    if (brights_vfs_mount_external("manual") == 0) {
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, "mounted at /dev/mnt/\n");
    } else {
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, "mount failed\n");
    }
    return 1;
  }
  if (streq(cmd, "clear")) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\x1b[2J\x1b[H");
    return 1;
  }
  if (streq(cmd, "runuser")) {
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "entering user mode\n");
    brights_userinit();
    return 0;
  }
  if (streq(cmd, "reboot")) {
    outb(0x64, 0xFE);
    return 0;
  }
  if (streq(cmd, "halt")) {
    return 0;
  }
  if (starts_with(cmd, "cat ")) {
    cmd_cat(cmd + 4);
    return 1;
  }
  if (starts_with(cmd, "stat ")) {
    cmd_stat(cmd + 5);
    return 1;
  }
  if (starts_with(cmd, "login ")) {
    cmd_login(cmd + 6);
    return 1;
  }
  if (starts_with(cmd, "passwd ")) {
    cmd_passwd(cmd + 7);
    return 1;
  }
  if (starts_with(cmd, "useradd ")) {
    cmd_useradd(cmd + 8);
    return 1;
  }
  if (starts_with(cmd, "setpf ")) {
    cmd_setpf(cmd + 6);
    return 1;
  }
  if (starts_with(cmd, "touch ")) {
    cmd_touch(cmd + 6);
    return 1;
  }
  if (starts_with(cmd, "write ")) {
    cmd_write(cmd + 6);
    return 1;
  }
  if (starts_with(cmd, "append ")) {
    cmd_append(cmd + 7);
    return 1;
  }
  if (starts_with(cmd, "rm ")) {
    cmd_rm(cmd + 3);
    return 1;
  }
  if (starts_with(cmd, "hexdump ")) {
    cmd_hexdump(cmd + 8);
    return 1;
  }
  if (starts_with(cmd, "echo ")) {
    cmd_echo(cmd + 5);
    return 1;
  }

  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "unknown command\n");
  return 1;
}

void brights_kshell_run(void)
{
  char line[KSHELL_MAX_LINE];
  int len = 0;

  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "brights shell ready\n");
  cmd_help();
  print_prompt();

  for (;;) {
    uint8_t ch = brights_serial_read_byte_blocking(BRIGHTS_COM1_PORT);

    if (ch == '\r' || ch == '\n') {
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\n");
      line[len] = 0;
      if (!handle_line(line)) {
        brights_serial_write_ascii(BRIGHTS_COM1_PORT, "bye\n");
        return;
      }
      len = 0;
      print_prompt();
      continue;
    }

    if ((ch == 0x08 || ch == 0x7F) && len > 0) {
      --len;
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\b \b");
      continue;
    }

    if (ch >= 32 && ch < 127 && len < KSHELL_MAX_LINE - 1) {
      line[len++] = (char)ch;
      char echo[2] = {(char)ch, 0};
      brights_serial_write_ascii(BRIGHTS_COM1_PORT, echo);
    }
  }
}
