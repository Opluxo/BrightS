#include "tty.h"
#include "serial.h"
#include "ps2kbd.h"
#include "usb.h"
#include "font.h"
#include "../kernel/printf.h"
#include "../kernel/core/kernel_util.h"

static brights_console_t tty_console;
static int tty_ready = 0;
static int tty_mode = TTY_MODE_COOKED;

#define TTY_LINE_BUF 256
static char line_buf[TTY_LINE_BUF];
static int line_len = 0;
static int line_ready = 0;
static int line_pos = 0;

void brights_tty_init(void)
{
  if (!tty_ready) {
    brights_serial_console_init(&tty_console, BRIGHTS_COM1_PORT);
    tty_ready = 1;
  }
}

void brights_tty_set_mode(int mode) { tty_mode = mode; }
int brights_tty_get_mode(void) { return tty_mode; }

void brights_tty_write(const uint16_t *s)
{
  if (!s) return;
  brights_tty_init();
  brights_print(&tty_console, s);
}

void brights_tty_write_str(const char *s)
{
  if (!s) return;
  brights_tty_init();
  while (*s) {
    char buf[2] = {*s++, 0};
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, buf);
  }
}

void brights_tty_write_char(char ch)
{
  char buf[2] = {ch, 0};
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, buf);
}

static void cooked_process_char(char ch)
{
  if (ch == '\b' || ch == 0x7F) {
    if (line_len > 0) {
      line_len--;
      brights_tty_write_char('\b');
      brights_tty_write_char(' ');
      brights_tty_write_char('\b');
    }
    return;
  }

  if (ch == '\n' || ch == '\r') {
    line_buf[line_len++] = '\n';
    line_buf[line_len] = 0;
    line_ready = 1;
    line_pos = 0;
    brights_tty_write_char('\n');
    return;
  }

  if (ch == 27) {
    line_len = 0;
    line_buf[0] = 0;
    brights_tty_write_char('\n');
    return;
  }

  if (ch == '\t') {
    for (int i = 0; i < 4 && line_len < TTY_LINE_BUF - 2; i++) {
      line_buf[line_len++] = ' ';
      brights_tty_write_char(' ');
    }
    return;
  }

  if (line_len < TTY_LINE_BUF - 2) {
    line_buf[line_len++] = ch;
    brights_tty_write_char(ch);
  }
}

int brights_tty_read_char(char *out_ch)
{
  if (!out_ch) return -1;
  brights_tty_init();

  if (tty_mode == TTY_MODE_COOKED) {
    if (line_ready && line_pos < line_len) {
      *out_ch = line_buf[line_pos++];
      if (line_pos >= line_len) {
        line_ready = line_len = line_pos = 0;
      }
      return 1;
    }

    char raw_ch;
    if (brights_ps2kbd_read_char(&raw_ch) > 0) cooked_process_char(raw_ch);

    brights_usb_hid_poll_all();
    uint8_t usb_ch;
    if (brights_usb_hid_read_key(&usb_ch) == 0) {
      cooked_process_char((char)usb_ch);
    }

    uint8_t serial_ch;
    if (brights_serial_read_byte(BRIGHTS_COM1_PORT, &serial_ch) > 0) {
      cooked_process_char((char)serial_ch);
    }

    if (line_ready && line_pos < line_len) {
      *out_ch = line_buf[line_pos++];
      if (line_pos >= line_len) {
        line_ready = line_len = line_pos = 0;
      }
      return 1;
    }
    return 0;
  }

  uint8_t serial_ch;
  if (brights_serial_read_byte(BRIGHTS_COM1_PORT, &serial_ch) > 0) {
    *out_ch = (char)serial_ch;
    return 1;
  }

  if (brights_ps2kbd_read_char(out_ch) > 0) return 1;

  brights_usb_hid_poll_all();
  uint8_t usb_ch;
  if (brights_usb_hid_read_key(&usb_ch) == 0) {
    *out_ch = (char)usb_ch;
    return 1;
  }

  return 0;
}

char brights_tty_read_char_blocking(void)
{
  char ch;
  while (brights_tty_read_char(&ch) <= 0) {
    __asm__ __volatile__("sti");
    __asm__ __volatile__("hlt");
    __asm__ __volatile__("cli");
  }
  return ch;
}

static fb_console_t fb_con = {0};
static int fb_con_initialized = 0;

void fb_console_set_work_area(int y, int h)
{
  fb_con.work_y = y;
  fb_con.work_h = h;
  if (fb_con.cursor_y < y || fb_con.cursor_y >= y + h) {
    fb_con.cursor_x = 0;
    fb_con.cursor_y = y;
  }
}

static void fb_console_write_strip_ansi(const char *s)
{
  if (!s) return;
  while (*s) {
    if (*s == '\033') {
      ++s;
      if (*s == '[') {
        ++s;
        while (*s && !((*s >= 'A' && *s <= 'Z') || (*s >= 'a' && *s <= 'z'))) {
          ++s;
        }
        if (*s) ++s;
      }
    } else {
      fb_console_put_char(*s);
      ++s;
    }
  }
}

void fb_console_init(void)
{
  if (fb_con_initialized) return;
  
  brights_fb_info_t *fb = brights_fb_get_info();
  if (!fb) return;
  
  brights_dbuffer_init();
  
  fb_con.width = fb->width / FONT_WIDTH;
  fb_con.height = fb->height / FONT_HEIGHT;
  fb_con.cursor_x = fb_con.cursor_y = 0;
  fb_con.tab_width = 4;
  fb_con.fg_color = brights_rgb(255, 255, 255);
  fb_con.bg_color = brights_rgb(0, 40, 80);
  fb_con.scroll_enabled = 1;
  fb_con.cursor_visible = 1;
  fb_con.work_y = 0;
  fb_con.work_h = fb_con.height;
  fb_con.utf8_len = 0;
  fb_con.utf8_expected = 0;
  
  fb_con_initialized = 1;
  fb_console_clear();
  brights_serial_output_hook = fb_console_write_strip_ansi;
}

void fb_console_clear(void)
{
  if (!fb_con_initialized) fb_console_init();
  brights_fb_fill(fb_con.bg_color);
  fb_con.cursor_x = 0;
  fb_con.cursor_y = fb_con.work_y;
}

void fb_console_set_colors(brights_color_t fg, brights_color_t bg)
{
  fb_con.fg_color = fg;
  fb_con.bg_color = bg;
}

void fb_console_goto(int x, int y)
{
  int y_limit = fb_con.work_y + fb_con.work_h;
  if (x < 0) x = 0;
  else if (x >= fb_con.width) x = fb_con.width - 1;
  if (y < fb_con.work_y) y = fb_con.work_y;
  else if (y >= y_limit) y = y_limit - 1;
  fb_con.cursor_x = x;
  fb_con.cursor_y = y;
}

void fb_console_get_pos(int *x, int *y)
{
  if (x) *x = fb_con.cursor_x;
  if (y) *y = fb_con.cursor_y;
}

void fb_console_newline(void)
{
  int limit = fb_con.work_y + fb_con.work_h;
  fb_con.cursor_x = 0;
  if (++fb_con.cursor_y >= limit) {
    if (fb_con.scroll_enabled) {
      fb_console_scroll();
      fb_con.cursor_y = limit - 1;
    } else {
      fb_con.cursor_y = fb_con.work_y;
    }
  }
}

void fb_console_scroll(void)
{
  brights_fb_info_t *fb = brights_fb_get_info();
  if (!fb) return;

  uint32_t line_h = 16;
  uint32_t work_start_px = fb_con.work_y * line_h;
  uint32_t scroll_bytes = line_h * fb->pitch;
  uint8_t *fb_base = (uint8_t *)fb->framebuffer;
  uint32_t copy_rows = (fb_con.work_h - 1) * line_h;
  uint32_t copy_bytes = copy_rows * fb->pitch;
  uint32_t src_off = work_start_px * fb->pitch + scroll_bytes;
  uint32_t dst_off = work_start_px * fb->pitch;

  for (uint32_t i = 0; i < copy_bytes; i++) {
    fb_base[dst_off + i] = fb_base[src_off + i];
  }

  uint8_t *clear_start = fb_base + dst_off + copy_bytes;
  for (uint32_t i = 0; i < scroll_bytes; i++) {
    clear_start[i] = 0;
  }
}

static void fb_console_render_codepoint(uint32_t cp)
{
  if (!fb_con_initialized) return;

  int px = fb_con.cursor_x * FONT_WIDTH;
  int py = fb_con.cursor_y * 16;
  uint32_t fg = (uint32_t)(fb_con.fg_color.r << 16 | fb_con.fg_color.g << 8 | fb_con.fg_color.b);
  uint32_t bg = (uint32_t)(fb_con.bg_color.r << 16 | fb_con.bg_color.g << 8 | fb_con.bg_color.b);

  brights_font_draw_codepoint(px, py, cp, fg, bg);
}

void fb_console_put_codepoint(uint32_t cp)
{
  if (!fb_con_initialized) fb_console_init();

  int w = kutil_codepoint_width(cp);
  if (w == 0) return;

  if (w == 2 && fb_con.cursor_x + 2 > fb_con.width) {
    fb_console_newline();
  }

  int px = fb_con.cursor_x * FONT_WIDTH;
  int py = fb_con.cursor_y * 16;

  if (cp == '\n') {
    fb_console_newline();
    return;
  }
  if (cp == '\r') {
    fb_con.cursor_x = 0;
    return;
  }
  if (cp == '\t') {
    fb_con.cursor_x = (fb_con.cursor_x + fb_con.tab_width) & ~(fb_con.tab_width - 1);
    if (fb_con.cursor_x >= fb_con.width) fb_console_newline();
    return;
  }
  if (cp == '\b') {
    if (fb_con.cursor_x > 0) {
      fb_con.cursor_x--;
      brights_fb_fill_rect(fb_con.cursor_x * FONT_WIDTH, py, FONT_WIDTH, FONT_HEIGHT, fb_con.bg_color);
    }
    return;
  }

  fb_console_render_codepoint(cp);
  fb_con.cursor_x += w;
  if (fb_con.cursor_x >= fb_con.width) fb_console_newline();
}

void fb_console_put_char(char c)
{
  if (!fb_con_initialized) fb_console_init();

  unsigned char uc = (unsigned char)c;

  switch (c) {
    case '\n':
    case '\r':
    case '\t':
    case '\b':
      /* Flush any incomplete UTF-8 sequence first */
      if (fb_con.utf8_len > 0) {
        for (int i = 0; i < fb_con.utf8_len; i++)
          fb_console_put_char(fb_con.utf8_buf[i]);
        fb_con.utf8_len = 0;
        fb_con.utf8_expected = 0;
      }
      fb_console_put_codepoint((uint32_t)uc);
      return;
    default:
      break;
  }

  /* ASCII fast path */
  if (uc < 0x80) {
    if (fb_con.utf8_len > 0) {
      for (int i = 0; i < fb_con.utf8_len; i++)
        fb_console_put_char(fb_con.utf8_buf[i]);
      fb_con.utf8_len = 0;
      fb_con.utf8_expected = 0;
    }
    fb_console_put_codepoint((uint32_t)uc);
    return;
  }

  /* UTF-8 continuation byte */
  if (kutil_utf8_is_continuation(uc) && fb_con.utf8_len > 0 && fb_con.utf8_len < fb_con.utf8_expected) {
    fb_con.utf8_buf[fb_con.utf8_len++] = c;
    if (fb_con.utf8_len >= fb_con.utf8_expected) {
      fb_con.utf8_buf[fb_con.utf8_len] = 0;
      int len;
      uint32_t cp = kutil_utf8_decode(fb_con.utf8_buf, &len);
      fb_console_put_codepoint(cp);
      fb_con.utf8_len = 0;
      fb_con.utf8_expected = 0;
    }
    return;
  }

  /* UTF-8 leading byte */
  if (fb_con.utf8_len > 0) {
    for (int i = 0; i < fb_con.utf8_len; i++)
      fb_console_put_char(fb_con.utf8_buf[i]);
    fb_con.utf8_len = 0;
    fb_con.utf8_expected = 0;
  }

  int expected = kutil_utf8_lead_len(uc);
  if (expected > 1) {
    fb_con.utf8_buf[0] = c;
    fb_con.utf8_len = 1;
    fb_con.utf8_expected = expected;
  } else {
    fb_console_put_codepoint((uint32_t)uc);
  }
}

void fb_console_write_str(const char *s)
{
  if (!s) return;
  while (*s) fb_console_put_char(*s++);
}

void fb_console_write_utf8(const char *s)
{
  if (!s) return;
  while (*s) fb_console_put_char(*s++);
}

void fb_console_write_line(const char *s)
{
  fb_console_write_str(s);
  fb_console_newline();
}

void fb_console_set_cursor_visible(int visible) { fb_con.cursor_visible = visible; }

void fb_console_update_cursor(void)
{
  if (!fb_con_initialized || !fb_con.cursor_visible) return;
  brights_fb_fill_rect(fb_con.cursor_x * 8, fb_con.cursor_y * 16, 8, 2, fb_con.fg_color);
}

void fb_console_flush(void)
{
  brights_dbuffer_flip();
}

fb_console_t *fb_console_get_info(void) { return &fb_con; }

static void fb_console_write_int(int value, int base)
{
  char buffer[32];
  char *p = buffer;
  int negative = 0;
  
  if (value < 0) {
    negative = 1;
    value = -value;
  }
  
  unsigned int uvalue = (unsigned int)value;
  
  if (base == 16) {
    static const char hex[] = "0123456789abcdef";
    if (uvalue == 0) {
      fb_console_put_char('0');
      return;
    }
    char tmp[16];
    int i = 0;
    while (uvalue) {
      tmp[i++] = hex[uvalue & 0xf];
      uvalue >>= 4;
    }
    while (i) fb_console_put_char(tmp[--i]);
    return;
  }
  
  if (uvalue == 0) {
    fb_console_put_char('0');
    return;
  }
  
  while (uvalue) {
    *p++ = "0123456789"[uvalue % base];
    uvalue /= base;
  }
  
  if (negative) fb_console_put_char('-');
  while (p > buffer) fb_console_put_char(*--p);
}

static void fb_console_write_str_va(const char *s)
{
  while (*s) fb_console_put_char(*s++);
}

void fb_printf(const char *fmt, ...)
{
  if (!fmt) return;
  
  const char *p = fmt;
  while (*p) {
    if (*p == '%') {
      p++;
      switch (*p) {
        case 'd':
        case 'i':
          fb_console_write_int(0, 10);
          break;
        case 'u':
        case 'x':
        case 'X':
          fb_console_write_int(0, 16);
          break;
        case 'p':
          fb_console_write_str_va("0x");
          fb_console_write_int(0, 16);
          break;
        case 'c':
          fb_console_put_char(0);
          break;
        case 's':
          fb_console_write_str_va("(null)");
          break;
        case '%':
          fb_console_put_char('%');
          break;
        default:
          break;
      }
    } else {
      fb_console_put_char(*p);
    }
    p++;
  }
}

void fb_printf_color(brights_color_t fg, brights_color_t bg, const char *fmt, ...)
{
  if (!fmt) return;
  
  brights_color_t old_fg = fb_con.fg_color;
  brights_color_t old_bg = fb_con.bg_color;
  
  fb_con.fg_color = fg;
  fb_con.bg_color = bg;
  fb_printf(fmt);
  
  fb_con.fg_color = old_fg;
  fb_con.bg_color = old_bg;
}
