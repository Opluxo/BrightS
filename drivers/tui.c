#include "tui.h"
#include "tty.h"
#include "fb.h"
#include "font.h"
#include "theme.h"
#include "../kernel/core/kernel_util.h"
#include "../kernel/core/clock.h"
#include "../kernel/core/sched.h"
#include "../kernel/core/proc.h"
#include "../kernel/core/pmem.h"
#include "../drivers/rtc.h"
#include <stdint.h>

static int tui_initialized = 0;

/* ---- Toast notification state ---- */
typedef struct {
  int type;
  int ttl;                /* ticks remaining; 0 = slot empty */
  char title[32];
  char message[64];
} tui_toast_t;

static tui_toast_t toasts[TUI_TOAST_MAX];

/* ---- Text helpers ---- */

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

static void draw_shadow(int x, int y, int w, int h, int r, brights_color_t base)
{
  brights_color_t shadow = brights_color_darken(base, 70);
  brights_fb_fill_rounded_rect(x + TUI_SHADOW_OFFSET, y + TUI_SHADOW_OFFSET,
    w, h, r, shadow);
}

static void int_to_str(uint64_t val, char *buf, int *idx)
{
  char tmp[20];
  int n = 0;
  if (val == 0) { tmp[n++] = '0'; }
  else {
    while (val > 0) { tmp[n++] = (char)('0' + val % 10); val /= 10; }
  }
  for (int i = n - 1; i >= 0; i--) buf[(*idx)++] = tmp[i];
}

/* ---- Legacy color accessors (backward compat) ---- */

brights_color_t tui_clr_black(void)           { return brights_rgb(0, 0, 0); }
brights_color_t tui_clr_red(void)             { return brights_rgb(170, 0, 0); }
brights_color_t tui_clr_green(void)           { return brights_rgb(0, 170, 0); }
brights_color_t tui_clr_yellow(void)          { return brights_rgb(170, 170, 0); }
brights_color_t tui_clr_blue(void)            { return brights_rgb(0, 0, 170); }
brights_color_t tui_clr_magenta(void)         { return brights_rgb(170, 0, 170); }
brights_color_t tui_clr_cyan(void)            { return brights_rgb(0, 170, 170); }
brights_color_t tui_clr_white(void)           { return brights_rgb(200, 200, 200); }
brights_color_t tui_clr_bright_black(void)    { return brights_rgb(85, 85, 85); }
brights_color_t tui_clr_bright_red(void)      { return brights_rgb(255, 85, 85); }
brights_color_t tui_clr_bright_green(void)    { return brights_rgb(85, 255, 85); }
brights_color_t tui_clr_bright_yellow(void)   { return brights_rgb(255, 255, 85); }
brights_color_t tui_clr_bright_blue(void)     { return brights_rgb(85, 85, 255); }
brights_color_t tui_clr_bright_magenta(void)  { return brights_rgb(255, 85, 255); }
brights_color_t tui_clr_bright_cyan(void)     { return brights_rgb(85, 255, 255); }
brights_color_t tui_clr_bright_white(void)    { return brights_rgb(255, 255, 255); }

/* ---- Init ---- */

void tui_init(void)
{
  if (tui_initialized) return;
  brights_theme_init();
  fb_console_init();
  for (int i = 0; i < TUI_TOAST_MAX; i++) toasts[i].ttl = 0;
  tui_initialized = 1;
}

void tui_apply_theme(void)
{
  /* Redraw both bars with current theme (called after theme switch) */
  tui_draw_title_bar("BrightS v0.1.3.4", NULL);
  tui_draw_status_bar("", "");
}

/* ---- Title bar ---- */

void tui_draw_title_bar(const char *left, const char *right)
{
  brights_fb_info_t *fb = brights_fb_get_info();
  if (!fb) return;
  const brights_theme_colors_t *th = brights_theme_get();

  int fb_w = (int)fb->width;

  /* Shadow */
  draw_shadow(0, 0, fb_w, TUI_BAR_H + 4, TUI_BAR_RADIUS, th->shadow);

  /* Background: rounded rect + horizontal gradient */
  brights_fb_fill_rounded_rect(0, 0, fb_w, TUI_BAR_H + 4, TUI_BAR_RADIUS, th->bar_bg);
  brights_fb_fill_gradient_h(0, 0, fb_w, TUI_BAR_H + 4, th->bar_bg, th->bar_bg_alt);

  /* Top accent line */
  brights_fb_draw_hline(TUI_BAR_RADIUS, 0, fb_w - TUI_BAR_RADIUS * 2, th->bar_accent);

  /* Bottom edge */
  brights_fb_draw_hline(TUI_BAR_RADIUS, TUI_BAR_H + 3,
    fb_w - TUI_BAR_RADIUS * 2, th->bar_edge);

  /* Left text */
  if (left) {
    int ty = (TUI_BAR_H - 16) / 2;
    draw_text_px(TUI_BAR_PAD, ty, left, th->text_primary, 0x00000000);
  }

  /* Right text */
  if (right) {
    int tw = text_pixel_width(right);
    int tx = fb_w - TUI_BAR_PAD - tw;
    int ty = (TUI_BAR_H - 16) / 2;
    if (tx > TUI_BAR_PAD)
      draw_text_px(tx, ty, right, th->text_accent, 0x00000000);
  }

  /* Set console work area */
  fb_console_t *con = fb_console_get_info();
  if (con) {
    int bar_rows = (TUI_BAR_H + 15) / 16;
    fb_console_set_work_area(bar_rows, con->height - bar_rows - 1);
  }
}

/* ---- Status bar ---- */

void tui_draw_status_bar(const char *left, const char *right)
{
  brights_fb_info_t *fb = brights_fb_get_info();
  if (!fb) return;
  const brights_theme_colors_t *th = brights_theme_get();

  int fb_w = (int)fb->width;
  int fb_h = (int)fb->height;
  int bar_y = fb_h - TUI_BAR_H - 4;

  /* Shadow */
  draw_shadow(0, bar_y - TUI_SHADOW_OFFSET, fb_w, TUI_BAR_H + 4,
    TUI_BAR_RADIUS, th->shadow);

  /* Background */
  brights_fb_fill_rounded_rect(0, bar_y, fb_w, TUI_BAR_H + 4, TUI_BAR_RADIUS,
    th->bar_bg);
  brights_fb_fill_gradient_h(0, bar_y, fb_w, TUI_BAR_H + 4,
    th->bar_bg, th->bar_bg_alt);

  /* Top accent */
  brights_fb_draw_hline(TUI_BAR_RADIUS, bar_y, fb_w - TUI_BAR_RADIUS * 2,
    th->bar_accent);

  /* Bottom edge */
  brights_fb_draw_hline(TUI_BAR_RADIUS, bar_y + TUI_BAR_H + 3,
    fb_w - TUI_BAR_RADIUS * 2, th->bar_edge);

  /* Left text */
  if (left) {
    int ty = bar_y + (TUI_BAR_H - 16) / 2;
    draw_text_px(TUI_BAR_PAD, ty, left, th->success, 0x00000000);
  }

  /* Right text */
  if (right) {
    int tw = text_pixel_width(right);
    int tx = fb_w - TUI_BAR_PAD - tw;
    int ty = bar_y + (TUI_BAR_H - 16) / 2;
    if (tx > TUI_BAR_PAD)
      draw_text_px(tx, ty, right, th->text_accent, 0x00000000);
  }
}

void tui_refresh_status(const char *user, int is_root)
{
  brights_fb_info_t *fb = brights_fb_get_info();
  if (!fb) return;
  const brights_theme_colors_t *th = brights_theme_get();

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
  if (is_root) {
    lbuf[i++] = ' ';
    lbuf[i++] = '#';
  } else {
    lbuf[i++] = ' ';
    lbuf[i++] = '$';
  }
  lbuf[i] = 0;

  /* Format right: "HH:MM:SS | used/total MB | N proc" */
  char rbuf[128];
  int ri = 0;

  /* Time */
  brights_rtc_time_t tr;
  if (brights_rtc_read(&tr) == 0) {
    rbuf[ri++] = (char)('0' + tr.hour / 10);
    rbuf[ri++] = (char)('0' + tr.hour % 10);
    rbuf[ri++] = ':';
    rbuf[ri++] = (char)('0' + tr.minute / 10);
    rbuf[ri++] = (char)('0' + tr.minute % 10);
    rbuf[ri++] = ':';
    rbuf[ri++] = (char)('0' + tr.second / 10);
    rbuf[ri++] = (char)('0' + tr.second % 10);
  }

  /* Separator */
  rbuf[ri++] = ' '; rbuf[ri++] = 0xE2; rbuf[ri++] = 0x96; rbuf[ri++] = 0x92;
                  rbuf[ri++] = ' '; /* bullet separator */

  /* Memory */
  uint64_t used_mb = brights_pmem_used_bytes() / (1024 * 1024);
  uint64_t total_mb = brights_pmem_total_bytes() / (1024 * 1024);
  int_to_str(used_mb, rbuf, &ri);
  rbuf[ri++] = '/'; rbuf[ri++] = 'M'; rbuf[ri++] = 'i';
  rbuf[ri++] = 'B';
  rbuf[ri++] = ' ';

  /* Separator */
  rbuf[ri++] = 0xE2; rbuf[ri++] = 0x96; rbuf[ri++] = 0x92; rbuf[ri++] = ' ';

  /* Process count */
  uint32_t pcount = brights_proc_total();
  int_to_str(pcount, rbuf, &ri);
  rbuf[ri++] = 'p'; rbuf[ri++] = 'r'; rbuf[ri++] = 'o'; rbuf[ri++] = 'c';
  rbuf[ri] = 0;

  tui_draw_status_bar(lbuf, rbuf);
  fb_console_flush();
}

/* ---- Dialog helpers ---- */

void tui_draw_dialog(int cx, int cy, int w, int h, const char *title,
                     brights_color_t border_color)
{
  const brights_theme_colors_t *th = brights_theme_get();

  /* Shadow */
  draw_shadow(cx - w / 2, cy - h / 2, w, h, 8, th->shadow);

  /* Background */
  brights_fb_fill_rounded_rect(cx - w / 2, cy - h / 2, w, h, 8, th->bg_secondary);

  /* Border */
  brights_fb_draw_rounded_rect(cx - w / 2, cy - h / 2, w, h, 8, border_color);

  /* Top accent line */
  brights_fb_draw_hline(8, cy - h / 2 + 1, w - 16,
    brights_color_lighten(border_color, 40));

  /* Title */
  if (title) {
    int tw = text_pixel_width(title);
    int tx = cx - tw / 2;
    int ty = cy - h / 2 + 12;
    draw_text_px(tx, ty, title, th->text_primary, 0x00000000);
  }

  /* Separator line below title */
  brights_fb_draw_hline(cx - w / 2 + 16, cy - h / 2 + 32, w - 32, th->border);
}

void tui_draw_input_field(int x, int y, int w, int h,
                          const char *label, int focused)
{
  const brights_theme_colors_t *th = brights_theme_get();

  /* Field background */
  brights_fb_fill_rounded_rect(x, y, w, h, 4,
    focused ? th->bg_tertiary : brights_color_darken(th->bg_tertiary, 15));

  /* Border */
  brights_color_t bc = focused ? th->border_focus : th->border;
  brights_fb_draw_rounded_rect(x, y, w, h, 4, bc);

  /* Label */
  if (label) {
    int ty = y + (h - 16) / 2;
    draw_text_px(x + 10, ty, label,
      focused ? th->text_accent : th->text_secondary, 0x00000000);
  }

  /* Focus glow (subtle top line) */
  if (focused) {
    brights_fb_draw_hline(x + 4, y + 1, w - 8, th->bar_accent);
  }
}

/* ---- Toast notifications ---- */

void tui_toast(int type, const char *title, const char *message)
{
  /* Find empty slot */
  for (int i = 0; i < TUI_TOAST_MAX; i++) {
    if (toasts[i].ttl == 0) {
      toasts[i].type = type;
      toasts[i].ttl = TUI_TOAST_DURATION;
      /* Copy title */
      int j = 0;
      if (title) {
        for (; title[j] && j < (int)sizeof(toasts[i].title) - 1; j++)
          toasts[i].title[j] = title[j];
      }
      toasts[i].title[j] = 0;
      /* Copy message */
      j = 0;
      if (message) {
        for (; message[j] && j < (int)sizeof(toasts[i].message) - 1; j++)
          toasts[i].message[j] = message[j];
      }
      toasts[i].message[j] = 0;
      return;
    }
  }
  /* All slots full — replace oldest */
  int oldest = 0;
  for (int i = 1; i < TUI_TOAST_MAX; i++) {
    if (toasts[i].ttl < toasts[oldest].ttl) oldest = i;
  }
  toasts[oldest].type = type;
  toasts[oldest].ttl = TUI_TOAST_DURATION;
  int j = 0;
  if (title) { for (; title[j] && j < 31; j++) toasts[oldest].title[j] = title[j]; }
  toasts[oldest].title[j] = 0;
  j = 0;
  if (message) { for (; message[j] && j < 63; j++) toasts[oldest].message[j] = message[j]; }
  toasts[oldest].message[j] = 0;
}

void tui_toast_update(void)
{
  for (int i = 0; i < TUI_TOAST_MAX; i++) {
    if (toasts[i].ttl > 0) toasts[i].ttl--;
  }
}

void tui_toast_draw(void)
{
  brights_fb_info_t *fb = brights_fb_get_info();
  if (!fb) return;
  const brights_theme_colors_t *th = brights_theme_get();

  int fb_w = (int)fb->width;
  int count = 0;

  for (int i = 0; i < TUI_TOAST_MAX; i++) {
    if (toasts[i].ttl == 0) continue;

    /* Slide-in from right during first 15 ticks */
    int x_offset = 0;
    if (toasts[i].ttl > TUI_TOAST_DURATION - 15)
      x_offset = (TUI_TOAST_DURATION - toasts[i].ttl) * (TUI_TOAST_WIDTH + 20) / 15;

    /* Fade-out during last 30 ticks */
    int alpha = 255;
    if (toasts[i].ttl < 30)
      alpha = (toasts[i].ttl * 255) / 30;

    int tx = fb_w - TUI_TOAST_WIDTH - 16 + x_offset;
    int ty = 56 + count * (TUI_TOAST_HEIGHT + TUI_TOAST_GAP);

    /* Color by type */
    brights_color_t accent;
    switch (toasts[i].type) {
      case TUI_TOAST_SUCCESS: accent = th->success; break;
      case TUI_TOAST_WARNING: accent = th->warning; break;
      case TUI_TOAST_ERROR:   accent = th->error;   break;
      default:                accent = th->info;     break;
    }

    /* Shadow */
    brights_fb_fill_rounded_rect(tx + 3, ty + 3, TUI_TOAST_WIDTH, TUI_TOAST_HEIGHT,
      6, brights_rgba(0, 0, 0, (uint8_t)(alpha / 3)));

    /* Background */
    brights_fb_fill_rounded_rect(tx, ty, TUI_TOAST_WIDTH, TUI_TOAST_HEIGHT,
      6, brights_rgba(th->bg_secondary.r, th->bg_secondary.g, th->bg_secondary.b,
                       (uint8_t)alpha));

    /* Left accent stripe */
    brights_fb_fill_rounded_rect(tx, ty, 4, TUI_TOAST_HEIGHT, 2,
      brights_rgba(accent.r, accent.g, accent.b, (uint8_t)alpha));

    /* Title */
    if (toasts[i].title[0]) {
      draw_text_px(tx + 14, ty + 8, toasts[i].title,
        brights_rgba(accent.r, accent.g, accent.b, (uint8_t)alpha), 0x00000000);
    }

    /* Message */
    if (toasts[i].message[0]) {
      draw_text_px(tx + 14, ty + 28, toasts[i].message,
        brights_rgba(th->text_secondary.r, th->text_secondary.g, th->text_secondary.b,
                     (uint8_t)alpha), 0x00000000);
    }

    count++;
  }
}

/* ---- Clear ---- */

void tui_clear(void)
{
  fb_console_clear();
  fb_console_t *con = fb_console_get_info();
  if (!con) return;
  int saved_y = con->cursor_y;
  tui_draw_title_bar("BrightS v0.1.3.4", NULL);
  con->cursor_y = saved_y;
}
