#include "tui.h"
#include "tty.h"
#include "fb.h"
#include "font.h"
#include "../kernel/core/kernel_util.h"
#include <stdint.h>

static int tui_initialized = 0;

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

static void draw_text_on_bar(int x, int y, const char *text, brights_color_t fg, brights_color_t bg)
{
  if (!text) return;
  int px = x * 8;
  int py = y * 16;
  uint32_t fg32 = (uint32_t)(fg.r << 16 | fg.g << 8 | fg.b);
  uint32_t bg32 = (uint32_t)(bg.r << 16 | bg.g << 8 | bg.b);
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
        brights_fb_info_t *fb = brights_fb_get_info();
        if (fb) {
          uint32_t *framebuffer = (uint32_t *)fb->framebuffer;
          uint32_t stride = fb->pitch / 4;
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
                else if (bg32 != 0xFFFFFFFF)
                  framebuffer[gpy * stride + gpx] = bg32;
              }
            }
          }
        }
      } else {
        brights_font_draw_char(px, py, '?', fg32, bg32);
      }
    }
    px += w * 8;
    pos += len;
  }
}

static void draw_bar_line(int y, brights_color_t bg)
{
  brights_fb_info_t *fb = brights_fb_get_info();
  if (!fb) return;
  brights_fb_fill_rect(0, y * 16, fb->width, 16, bg);
}

static void draw_separator(int y, brights_color_t color)
{
  brights_fb_info_t *fb = brights_fb_get_info();
  if (!fb) return;
  int py = y * 16 - 1;
  brights_fb_draw_hline(0, py, fb->width, color);
}

void tui_init(void)
{
  if (tui_initialized) return;
  fb_console_init();
  tui_initialized = 1;
}

void tui_draw_title_bar(const char *left, const char *right)
{
  brights_color_t bg = brights_rgb(0, 60, 120);
  brights_color_t fg = tui_clr_bright_white();
  brights_fb_info_t *fb = brights_fb_get_info();
  if (!fb) return;

  int cols = fb->width / 8;
  draw_bar_line(0, bg);
  draw_text_on_bar(1, 0, left, fg, bg);

  if (right) {
    int rlen = kutil_utf8_strwidth(right);
    int rx = cols - 1 - rlen;
    if (rx > 0) draw_text_on_bar(rx, 0, right, tui_clr_bright_cyan(), bg);
  }

  draw_separator(1, brights_rgb(0, 100, 180));
}

void tui_draw_status_bar(const char *left, const char *right)
{
  brights_color_t bg = brights_rgb(0, 30, 60);
  brights_fb_info_t *fb = brights_fb_get_info();
  if (!fb) return;

  fb_console_t *con = fb_console_get_info();
  int status_y = con->work_y + con->work_h;
  int cols = fb->width / 8;

  draw_bar_line(status_y, bg);

  if (left) draw_text_on_bar(1, status_y, left, tui_clr_bright_green(), bg);
  if (right) {
    int rlen = kutil_utf8_strwidth(right);
    int rx = cols - 1 - rlen;
    if (rx > 0) draw_text_on_bar(rx, status_y, right, tui_clr_bright_cyan(), bg);
  }
}

void tui_clear(void)
{
  fb_console_clear();
  fb_console_t *con = fb_console_get_info();
  if (!con) return;
  int saved_y = con->cursor_y;
  tui_draw_title_bar("BrightS v0.1.2.9", NULL);
  con->cursor_y = saved_y;
}
