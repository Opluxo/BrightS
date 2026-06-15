#include "tui.h"
#include "tty.h"
#include "fb.h"
#include "font.h"
#include "../kernel/core/kernel_util.h"
#include "../kernel/core/clock.h"
#include "../kernel/core/sched.h"
#include "../kernel/core/proc.h"
#include "../kernel/core/pmem.h"
#include "../drivers/rtc.h"
#include <stdint.h>

static int tui_initialized = 0;

/* Bar heights in pixels */
#define TUI_BAR_H       40
#define TUI_BAR_PAD     12
#define TUI_BAR_RADIUS  6
#define TUI_SHADOW_OFFSET 4
#define TUI_SHADOW_RADIUS 8

brights_color_t tui_clr_black(void)        { return brights_rgb(0, 0, 0); }
brights_color_t tui_clr_red(void)          { return brights_rgb(170, 0, 0); }
brights_color_t tui_clr_green(void)        { return brights_rgb(0, 170, 0); }
brights_color_t tui_clr_yellow(void)       { return brights_rgb(170, 170, 0); }
brights_color_t tui_clr_blue(void)         { return brights_rgb(0, 0, 170); }
brights_color_t tui_clr_magenta(void)      { return brights_rgb(170, 0, 170); }
brights_color_t tui_clr_cyan(void)         { return brights_rgb(0, 170, 170); }
brights_color_t tui_clr_white(void)        { return brights_rgb(200, 200, 200); }

brights_color_t tui_clr_bright_black(void)    { return brights_rgb(85, 85, 85); }
brights_color_t tui_clr_bright_red(void)      { return brights_rgb(255, 85, 85); }
brights_color_t tui_clr_bright_green(void)    { return brights_rgb(85, 255, 85); }
brights_color_t tui_clr_bright_yellow(void)   { return brights_rgb(255, 255, 85); }
brights_color_t tui_clr_bright_blue(void)     { return brights_rgb(85, 85, 255); }
brights_color_t tui_clr_bright_magenta(void)  { return brights_rgb(255, 85, 255); }
brights_color_t tui_clr_bright_cyan(void)     { return brights_rgb(85, 255, 255); }
brights_color_t tui_clr_bright_white(void)    { return brights_rgb(255, 255, 255); }

static void draw_text_px(int px, int py, const char *text, brights_color_t fg, uint32_t bg32)
{
  if (!text) return;
  uint32_t fg32 = (uint32_t)(fg.r << 16 | fg.g << 8 | fg.b);
  int pos = 0;
  while (text[pos]) {
    int len;
    uint32_t cp = kutil_utf8_decode(text + pos, &len);
    int w = kutil_codepoint_width(cp);
    if (cp < 0x80) {
      brights_font_draw_char(px, py, (char)cp, fg32, bg32);
    } else {
      const uint8_t *glyph = brights_get_chinese_glyph(cp);
      if (glyph) {
        uint32_t *framebuffer = (uint32_t *)brights_fb_active_ptr();
        uint32_t stride = brights_fb_active_pitch() / 4;
        brights_fb_info_t *fb = brights_fb_get_info();
        if (fb) {
          for (int row = 0; row < 16; row++) {
            uint8_t row_data0 = glyph[row * 2];
            uint8_t row_data1 = glyph[row * 2 + 1];
            for (int col = 0; col < 16; col++) {
              int byte_idx = col / 8;
              int bit_idx = 7 - (col % 8);
              uint8_t bits = (byte_idx == 0) ? row_data0 : row_data1;
              int gpx = px + col;
              int gpy = py + row;
              if (gpx >= 0 && gpx < (int)fb->width && gpy >= 0 && gpy < (int)fb->height) {
                if (bits & (1 << bit_idx))
                  framebuffer[gpy * stride + gpx] = fg32;
              }
            }
          }
        }
      } else {
        brights_font_draw_char(px, py, '?', fg32, bg32);
      }
    }
    px += w * 16;
    pos += len;
  }
}

static int text_pixel_width(const char *text)
{
  if (!text) return 0;
  int pw = 0;
  int pos = 0;
  while (text[pos]) {
    int len;
    uint32_t cp = kutil_utf8_decode(text + pos, &len);
    pw += kutil_codepoint_width(cp) * 16;
    pos += len;
  }
  return pw;
}

static void draw_shadow_rounded_rect(int x, int y, int w, int h, int r, brights_color_t bg)
{
  brights_fb_fill_rounded_rect(x + TUI_SHADOW_OFFSET, y + TUI_SHADOW_OFFSET,
    w, h, r, brights_color_darken(bg, 60));
}

void tui_init(void)
{
  if (tui_initialized) return;
  fb_console_init();
  tui_initialized = 1;
}

void tui_draw_title_bar(const char *left, const char *right)
{
  brights_fb_info_t *fb = brights_fb_get_info();
  if (!fb) return;

  int fb_w = (int)fb->width;

  /* Shadow beneath title bar */
  brights_color_t shadow_bg = brights_rgb(5, 8, 20);
  draw_shadow_rounded_rect(0, 0, fb_w, TUI_BAR_H + 4, TUI_BAR_RADIUS, shadow_bg);

  /* Title bar background: horizontal gradient dark blue */
  brights_fb_fill_rounded_rect(0, 0, fb_w, TUI_BAR_H + 4, TUI_BAR_RADIUS,
    brights_rgb(10, 18, 45));
  brights_fb_fill_gradient_h(0, 0, fb_w, TUI_BAR_H + 4,
    brights_rgb(8, 16, 40), brights_rgb(14, 30, 65));

  /* Top accent line (cyan glow) */
  brights_fb_draw_hline(TUI_BAR_RADIUS, 0, fb_w - TUI_BAR_RADIUS * 2,
    brights_rgb(0, 200, 255));

  /* Bottom edge highlight */
  brights_fb_draw_hline(TUI_BAR_RADIUS, TUI_BAR_H + 3, fb_w - TUI_BAR_RADIUS * 2,
    brights_rgb(0, 100, 150));

  /* Left text (white, vertically centered) */
  if (left) {
    int tw = text_pixel_width(left);
    int tx = TUI_BAR_PAD;
    int ty = (TUI_BAR_H - 16) / 2;
    draw_text_px(tx, ty, left, tui_clr_bright_white(), 0x00000000);
  }

  /* Right text (cyan, right-aligned) */
  if (right) {
    int tw = text_pixel_width(right);
    int tx = fb_w - TUI_BAR_PAD - tw;
    int ty = (TUI_BAR_H - 16) / 2;
    if (tx > TUI_BAR_PAD) {
      draw_text_px(tx, ty, right, tui_clr_bright_cyan(), 0x00000000);
    }
  }

  /* Set fb_console work area below title bar */
  fb_console_t *con = fb_console_get_info();
  if (con) {
    int bar_rows = (TUI_BAR_H + 15) / 16;
    fb_console_set_work_area(bar_rows, con->height - bar_rows - 1);
  }
}

void tui_draw_status_bar(const char *left, const char *right)
{
  brights_fb_info_t *fb = brights_fb_get_info();
  if (!fb) return;

  int fb_w = (int)fb->width;
  int fb_h = (int)fb->height;
  int bar_y = fb_h - TUI_BAR_H - 4;

  /* Shadow above status bar */
  brights_color_t shadow_bg = brights_rgb(5, 8, 20);
  draw_shadow_rounded_rect(0, bar_y - TUI_SHADOW_OFFSET, fb_w, TUI_BAR_H + 4,
    TUI_BAR_RADIUS, shadow_bg);

  /* Status bar background: horizontal gradient dark blue */
  brights_fb_fill_rounded_rect(0, bar_y, fb_w, TUI_BAR_H + 4, TUI_BAR_RADIUS,
    brights_rgb(8, 14, 35));
  brights_fb_fill_gradient_h(0, bar_y, fb_w, TUI_BAR_H + 4,
    brights_rgb(6, 12, 30), brights_rgb(12, 24, 52));

  /* Top accent line */
  brights_fb_draw_hline(TUI_BAR_RADIUS, bar_y, fb_w - TUI_BAR_RADIUS * 2,
    brights_rgb(0, 160, 210));

  /* Bottom edge highlight */
  brights_fb_draw_hline(TUI_BAR_RADIUS, bar_y + TUI_BAR_H + 3,
    fb_w - TUI_BAR_RADIUS * 2, brights_rgb(0, 80, 120));

  /* Left text (green, vertically centered) */
  if (left) {
    int tx = TUI_BAR_PAD;
    int ty = bar_y + (TUI_BAR_H - 16) / 2;
    draw_text_px(tx, ty, left, tui_clr_bright_green(), 0x00000000);
  }

  /* Right text (cyan, right-aligned) */
  if (right) {
    int tw = text_pixel_width(right);
    int tx = fb_w - TUI_BAR_PAD - tw;
    int ty = bar_y + (TUI_BAR_H - 16) / 2;
    if (tx > TUI_BAR_PAD) {
      draw_text_px(tx, ty, right, tui_clr_bright_cyan(), 0x00000000);
    }
  }
}

void tui_refresh_status(const char *user, int is_root)
{
  brights_fb_info_t *fb = brights_fb_get_info();
  if (!fb) return;

  /* Format left: "user@brights" */
  char lbuf[64];
  int i = 0;
  if (user) {
    for (int j = 0; user[j] && i < (int)sizeof(lbuf) - 16; ++j)
      lbuf[i++] = user[j];
  }
  lbuf[i++] = '@';
  lbuf[i++] = 'b'; lbuf[i++] = 'r'; lbuf[i++] = 'i'; lbuf[i++] = 'g';
  lbuf[i++] = 'h'; lbuf[i++] = 't'; lbuf[i++] = 's';
  lbuf[i] = 0;

  /* Format right: "HH:MM:SS | used/totalMB | N proc" */
  char rbuf[96];
  int ri = 0;

  /* Time */
  brights_rtc_time_t tr;
  if (brights_rtc_read(&tr) == 0) {
    int v;
    v = tr.hour / 10;   rbuf[ri++] = (char)('0' + v);
    v = tr.hour % 10;   rbuf[ri++] = (char)('0' + v);
    rbuf[ri++] = ':';
    v = tr.minute / 10; rbuf[ri++] = (char)('0' + v);
    v = tr.minute % 10; rbuf[ri++] = (char)('0' + v);
    rbuf[ri++] = ':';
    v = tr.second / 10; rbuf[ri++] = (char)('0' + v);
    v = tr.second % 10; rbuf[ri++] = (char)('0' + v);
  }

  /* Memory */
  rbuf[ri++] = ' ';
  rbuf[ri++] = '|';
  rbuf[ri++] = ' ';
  uint64_t used_mb = brights_pmem_used_bytes() / (1024 * 1024);
  uint64_t total_mb = brights_pmem_total_bytes() / (1024 * 1024);
  if (used_mb >= 1000) {
    rbuf[ri++] = (char)('0' + (used_mb / 1000) % 10);
    rbuf[ri++] = (char)('0' + (used_mb / 100) % 10);
    rbuf[ri++] = (char)('0' + (used_mb / 10) % 10);
    rbuf[ri++] = (char)('0' + used_mb % 10);
  } else if (used_mb >= 100) {
    rbuf[ri++] = (char)('0' + (used_mb / 100) % 10);
    rbuf[ri++] = (char)('0' + (used_mb / 10) % 10);
    rbuf[ri++] = (char)('0' + used_mb % 10);
  } else if (used_mb >= 10) {
    rbuf[ri++] = (char)('0' + (used_mb / 10) % 10);
    rbuf[ri++] = (char)('0' + used_mb % 10);
  } else {
    rbuf[ri++] = (char)('0' + used_mb);
  }
  rbuf[ri++] = 'M';
  rbuf[ri++] = '/';
  if (total_mb >= 1000) {
    rbuf[ri++] = (char)('0' + (total_mb / 1000) % 10);
    rbuf[ri++] = (char)('0' + (total_mb / 100) % 10);
    rbuf[ri++] = (char)('0' + (total_mb / 10) % 10);
    rbuf[ri++] = (char)('0' + total_mb % 10);
  } else if (total_mb >= 100) {
    rbuf[ri++] = (char)('0' + (total_mb / 100) % 10);
    rbuf[ri++] = (char)('0' + (total_mb / 10) % 10);
    rbuf[ri++] = (char)('0' + total_mb % 10);
  } else if (total_mb >= 10) {
    rbuf[ri++] = (char)('0' + (total_mb / 10) % 10);
    rbuf[ri++] = (char)('0' + total_mb % 10);
  } else {
    rbuf[ri++] = (char)('0' + total_mb);
  }
  rbuf[ri++] = 'M';

  /* Process count */
  rbuf[ri++] = ' ';
  rbuf[ri++] = '|';
  rbuf[ri++] = ' ';
  uint32_t pcount = brights_proc_total();
  if (pcount >= 100) {
    rbuf[ri++] = (char)('0' + (pcount / 100) % 10);
    rbuf[ri++] = (char)('0' + (pcount / 10) % 10);
    rbuf[ri++] = (char)('0' + pcount % 10);
  } else if (pcount >= 10) {
    rbuf[ri++] = (char)('0' + (pcount / 10) % 10);
    rbuf[ri++] = (char)('0' + pcount % 10);
  } else {
    rbuf[ri++] = (char)('0' + pcount);
  }
  rbuf[ri++] = ' ';
  rbuf[ri++] = 'p';
  rbuf[ri++] = 'r';
  rbuf[ri++] = 'o';
  rbuf[ri++] = 'c';
  rbuf[ri] = 0;

  tui_draw_status_bar(lbuf, rbuf);
  fb_console_flush();
}

void tui_clear(void)
{
  fb_console_clear();
  fb_console_t *con = fb_console_get_info();
  if (!con) return;
  int saved_y = con->cursor_y;
  tui_draw_title_bar("BrightS v0.1.3.1", NULL);
  con->cursor_y = saved_y;
}
