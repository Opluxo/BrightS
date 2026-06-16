#include "tty.h"
#include "serial.h"
#include "ps2kbd.h"
#include "usb.h"
#include "font.h"
#include "theme.h"
#include "../kernel/printf.h"
#include "../kernel/core/kernel_util.h"
#include "../kernel/core/clock.h"
#include <stdarg.h>

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

/* ============================================================
 * Framebuffer console — fully on graphics layer via double buffer
 * ============================================================ */

static void fb_console_write_strip_ansi(const char *s);

static fb_console_t fb_con = {0};
static int fb_con_initialized = 0;

/* ANSI color tables */
static const brights_color_t ansi_fg_table[16] = {
  /*  0 black       */ { 30, 30, 35, 255 },
  /*  1 red         */ { 200, 60, 60, 255 },
  /*  2 green       */ { 60, 180, 75, 255 },
  /*  3 yellow      */ { 200, 175, 50, 255 },
  /*  4 blue        */ { 60, 100, 210, 255 },
  /*  5 magenta     */ { 175, 60, 175, 255 },
  /*  6 cyan        */ { 50, 175, 190, 255 },
  /*  7 white       */ { 210, 215, 225, 255 },
  /*  8 bright black*/ { 90, 95, 105, 255 },
  /*  9 bright red  */ { 245, 90, 90, 255 },
  /* 10 bright green*/ { 80, 220, 100, 255 },
  /* 11 bright yellow*/ { 245, 225, 80, 255 },
  /* 12 bright blue */ { 90, 130, 245, 255 },
  /* 13 bright mag  */ { 210, 100, 210, 255 },
  /* 14 bright cyan */ { 80, 210, 230, 255 },
  /* 15 bright white*/ { 245, 245, 250, 255 },
};

static const brights_color_t ansi_bg_table[16] = {
  /*  0 */ { 15, 15, 20, 255 },
  /*  1 */ { 140, 35, 35, 255 },
  /*  2 */ { 35, 120, 45, 255 },
  /*  3 */ { 140, 120, 30, 255 },
  /*  4 */ { 35, 60, 150, 255 },
  /*  5 */ { 120, 35, 120, 255 },
  /*  6 */ { 30, 115, 130, 255 },
  /*  7 */ { 150, 155, 165, 255 },
  /*  8 */ { 55, 58, 65, 255 },
  /*  9 */ { 185, 60, 60, 255 },
  /* 10 */ { 50, 160, 65, 255 },
  /* 11 */ { 180, 165, 50, 255 },
  /* 12 */ { 50, 80, 180, 255 },
  /* 13 */ { 155, 65, 155, 255 },
  /* 14 */ { 50, 155, 170, 255 },
  /* 15 */ { 185, 190, 200, 255 },
};

void fb_console_set_work_area(int y, int h)
{
  fb_con.work_y = y;
  fb_con.work_h = h;
  if (fb_con.cursor_y < y || fb_con.cursor_y >= y + h) {
    fb_con.cursor_x = 0;
    fb_con.cursor_y = y;
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
  fb_con.fg_color = brights_theme_text();
  fb_con.bg_color = brights_theme_bg();
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

/* ---- All rendering goes through double buffer ---- */

void fb_console_clear(void)
{
  if (!fb_con_initialized) fb_console_init();
  /* Theme-aware background */
  fb_con.bg_color = brights_theme_bg();
  brights_dbuffer_clear(fb_con.bg_color);
  brights_dbuffer_flip();
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

  /* Read from double buffer, write back to double buffer */
  uint8_t *buf = (uint8_t *)brights_fb_active_ptr();
  if (!buf) return;

  uint32_t copy_rows = (fb_con.work_h - 1) * line_h;
  uint32_t copy_bytes = copy_rows * fb->pitch;
  uint32_t src_off = work_start_px * fb->pitch + scroll_bytes;
  uint32_t dst_off = work_start_px * fb->pitch;

  /* Move pixel rows up */
  for (uint32_t i = 0; i < copy_bytes; i++) {
    buf[dst_off + i] = buf[src_off + i];
  }

  /* Clear bottom line with theme background */
  brights_color_t bg = brights_theme_bg();
  uint32_t bg32 = (bg.r << 16) | (bg.g << 8) | bg.b;
  uint32_t *pixels = (uint32_t *)buf;
  uint32_t stride = fb->pitch / 4;
  uint32_t clear_top = work_start_px + copy_rows;
  for (uint32_t row = 0; row < line_h; row++) {
    for (uint32_t col = 0; col < fb->width; col++) {
      pixels[(clear_top + row) * stride + col] = bg32;
    }
  }
}

static void fb_console_render_codepoint(uint32_t cp)
{
  if (!fb_con_initialized) return;

  int px = fb_con.cursor_x * FONT_WIDTH;
  int py = fb_con.cursor_y * 16;
  uint32_t fg = (uint32_t)(fb_con.fg_color.r << 16 | fb_con.fg_color.g << 8 | fb_con.fg_color.b);
  uint32_t bg = (uint32_t)(fb_con.bg_color.r << 16 | fb_con.bg_color.g << 8 | fb_con.bg_color.b);

  /* Draw character glyph into double buffer via brights_font_draw_codepoint */
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
      int px = fb_con.cursor_x * FONT_WIDTH;
      int py = fb_con.cursor_y * 16;
      uint32_t bg32 = (fb_con.bg_color.r << 16) | (fb_con.bg_color.g << 8) | fb_con.bg_color.b;
      /* Clear character cell in double buffer */
      brights_fb_info_t *fb = brights_fb_get_info();
      if (fb) {
        uint32_t *buf = (uint32_t *)brights_fb_active_ptr();
        uint32_t stride = fb->pitch / 4;
        for (int row = 0; row < FONT_HEIGHT; row++) {
          for (int col = 0; col < FONT_WIDTH; col++) {
            buf[(py + row) * stride + (px + col)] = bg32;
          }
        }
      }
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

  brights_fb_info_t *fb = brights_fb_get_info();
  if (!fb) return;

  uint32_t *buf = (uint32_t *)brights_fb_active_ptr();
  if (!buf) return;

  uint32_t stride = fb->pitch / 4;
  int cx = fb_con.cursor_x * FONT_WIDTH;
  int cy = fb_con.cursor_y * 16;

  /* Blink: toggle every ~30 ticks (~0.5s at 60Hz) */
  static int blink_counter = 0;
  static int blink_state = 1;
  blink_counter++;
  if (blink_counter >= 30) {
    blink_counter = 0;
    blink_state = !blink_state;
  }

  /* Draw a proper block cursor (8x14) with theme accent color */
  brights_color_t cursor_color = blink_state ? brights_theme_bar_accent() : brights_theme_bg();
  uint32_t c32 = (cursor_color.r << 16) | (cursor_color.g << 8) | cursor_color.b;

  int cw = FONT_WIDTH;
  int ch = FONT_HEIGHT - 2; /* slightly shorter than full cell */

  for (int row = 0; row < ch; row++) {
    for (int col = 0; col < cw; col++) {
      int px = cx + col;
      int py = cy + row;
      if (px >= 0 && px < (int)fb->width && py >= 0 && py < (int)fb->height) {
        buf[py * stride + px] = c32;
      }
    }
  }
}

void fb_console_flush(void)
{
  brights_dbuffer_flip();
}

fb_console_t *fb_console_get_info(void) { return &fb_con; }

/* ============================================================
 * ANSI SGR (Select Graphic Rendition) parsing
 * Handles: 30-37/40-47 (8-color fg/bg), 90-97/100-107 (bright fg/bg),
 *          0 (reset), 1 (bold/bright), 22 (normal), 39/49 (default fg/bg)
 * ============================================================ */

static void fb_console_parse_sgr(const char *params, int param_len)
{
  int code = 0;
  int has_code = 0;

  /* Parse the numeric parameter */
  for (int i = 0; i < param_len; i++) {
    if (params[i] >= '0' && params[i] <= '9') {
      code = code * 10 + (params[i] - '0');
      has_code = 1;
    }
    if (params[i] == ';' || i == param_len - 1) {
      if (!has_code) code = 0;

      /* Apply SGR code */
      if (code == 0) {
        fb_con.fg_color = brights_theme_text();
        fb_con.bg_color = brights_theme_bg();
      } else if (code == 1) {
        fb_con.fg_color = brights_color_lighten(fb_con.fg_color, 30);
      } else if (code == 22) {
        fb_con.fg_color = brights_theme_text();
      } else if (code >= 30 && code <= 37) {
        fb_con.fg_color = ansi_fg_table[code - 30];
      } else if (code >= 40 && code <= 47) {
        fb_con.bg_color = ansi_bg_table[code - 40];
      } else if (code >= 90 && code <= 97) {
        fb_con.fg_color = ansi_fg_table[code - 90 + 8];
      } else if (code >= 100 && code <= 107) {
        fb_con.bg_color = ansi_bg_table[code - 100 + 8];
      } else if (code == 39) {
        fb_con.fg_color = brights_theme_text();
      } else if (code == 49) {
        fb_con.bg_color = brights_theme_bg();
      }

      code = 0;
      has_code = 0;
    }
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
        /* Collect parameter bytes */
        const char *param_start = s;
        while (*s && *s >= '0' && *s <= '?' ) ++s;
        int param_len = (int)(s - param_start);
        /* Check for 'm' (SGR) */
        if (*s == 'm') {
          fb_console_parse_sgr(param_start, param_len);
          ++s;
        } else {
          /* Skip other CSI sequences */
          if (*s) ++s;
        }
      }
    } else {
      fb_console_put_char(*s);
      ++s;
    }
  }
}

/* ============================================================
 * printf-style framebuffer console output
 * ============================================================ */

static void fb_console_write_str_va(const char *s)
{
  while (*s) fb_console_put_char(*s++);
}

static void fb_console_write_unsigned(uint64_t val, int base)
{
  char buffer[24];
  int i = 0;
  if (val == 0) { fb_console_put_char('0'); return; }
  while (val > 0) { buffer[i++] = "0123456789abcdef"[val % base]; val /= base; }
  while (i > 0) fb_console_put_char(buffer[--i]);
}

void fb_printf(const char *fmt, ...)
{
  if (!fmt) return;

  va_list ap;
  va_start(ap, fmt);

  const char *p = fmt;
  while (*p) {
    if (*p == '%') {
      p++;
      /* Skip flags: -, +, 0, space, # */
      while (*p == '-' || *p == '+' || *p == '0' || *p == ' ' || *p == '#') p++;
      /* Skip width */
      while (*p >= '0' && *p <= '9') p++;
      /* Skip length modifier */
      if (*p == 'l') { p++; if (*p == 'l') p++; }
      if (*p == 'z') p++;

      switch (*p) {
        case 'd':
        case 'i': {
          int64_t val = va_arg(ap, int64_t);
          if (val < 0) { fb_console_put_char('-'); val = -val; }
          fb_console_write_unsigned((uint64_t)val, 10);
          break;
        }
        case 'u':
          fb_console_write_unsigned(va_arg(ap, uint64_t), 10);
          break;
        case 'x':
        case 'X':
          fb_console_write_unsigned(va_arg(ap, uint64_t), 16);
          break;
        case 'p': {
          uint64_t val = (uint64_t)(uintptr_t)va_arg(ap, void *);
          fb_console_write_str_va("0x");
          fb_console_write_unsigned(val, 16);
          break;
        }
        case 'c':
          fb_console_put_char((char)va_arg(ap, int));
          break;
        case 's': {
          const char *s = va_arg(ap, const char *);
          if (s) fb_console_write_str_va(s);
          else fb_console_write_str_va("(null)");
          break;
        }
        case '%':
          fb_console_put_char('%');
          break;
        default:
          fb_console_put_char('%');
          fb_console_put_char(*p);
          break;
      }
    } else {
      fb_console_put_char(*p);
    }
    p++;
  }

  va_end(ap);
}

void fb_printf_color(brights_color_t fg, brights_color_t bg, const char *fmt, ...)
{
  if (!fmt) return;

  brights_color_t old_fg = fb_con.fg_color;
  brights_color_t old_bg = fb_con.bg_color;

  fb_con.fg_color = fg;
  fb_con.bg_color = bg;

  va_list ap;
  va_start(ap, fmt);

  const char *p = fmt;
  while (*p) {
    if (*p == '%') {
      p++;
      while (*p == '-' || *p == '+' || *p == '0' || *p == ' ' || *p == '#') p++;
      while (*p >= '0' && *p <= '9') p++;
      if (*p == 'l') { p++; if (*p == 'l') p++; }
      if (*p == 'z') p++;

      switch (*p) {
        case 'd':
        case 'i': {
          int64_t val = va_arg(ap, int64_t);
          if (val < 0) { fb_console_put_char('-'); val = -val; }
          fb_console_write_unsigned((uint64_t)val, 10);
          break;
        }
        case 'u':
          fb_console_write_unsigned(va_arg(ap, uint64_t), 10);
          break;
        case 'x':
        case 'X':
          fb_console_write_unsigned(va_arg(ap, uint64_t), 16);
          break;
        case 'p': {
          uint64_t val = (uint64_t)(uintptr_t)va_arg(ap, void *);
          fb_console_write_str_va("0x");
          fb_console_write_unsigned(val, 16);
          break;
        }
        case 'c':
          fb_console_put_char((char)va_arg(ap, int));
          break;
        case 's': {
          const char *s = va_arg(ap, const char *);
          if (s) fb_console_write_str_va(s);
          else fb_console_write_str_va("(null)");
          break;
        }
        case '%':
          fb_console_put_char('%');
          break;
        default:
          fb_console_put_char('%');
          fb_console_put_char(*p);
          break;
      }
    } else {
      fb_console_put_char(*p);
    }
    p++;
  }

  va_end(ap);
  fb_con.fg_color = old_fg;
  fb_con.bg_color = old_bg;
}
