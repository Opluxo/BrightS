#ifndef BRIGHTS_TUI_H
#define BRIGHTS_TUI_H

#include <stdint.h>
#include "fb.h"

/* Bar geometry */
#define TUI_BAR_H       40
#define TUI_BAR_PAD     12
#define TUI_BAR_RADIUS  6
#define TUI_SHADOW_OFFSET 4
#define TUI_SHADOW_RADIUS 8

/* Toast notification */
#define TUI_TOAST_MAX       4
#define TUI_TOAST_DURATION  180   /* ticks (~3 seconds at 60Hz) */
#define TUI_TOAST_WIDTH     320
#define TUI_TOAST_HEIGHT    48
#define TUI_TOAST_GAP       8

/* Toast types */
#define TUI_TOAST_INFO      0
#define TUI_TOAST_SUCCESS   1
#define TUI_TOAST_WARNING   2
#define TUI_TOAST_ERROR     3

/* ---- Core ---- */
void tui_init(void);
void tui_clear(void);

/* ---- Bars ---- */
void tui_draw_title_bar(const char *left, const char *right);
void tui_draw_status_bar(const char *left, const char *right);
void tui_refresh_status(const char *user, int is_root);

/* ---- Dialog helpers ---- */
void tui_draw_dialog(int cx, int cy, int w, int h, const char *title,
                     brights_color_t border_color);
void tui_draw_input_field(int x, int y, int w, int h,
                          const char *label, int focused);

/* ---- Toast notifications ---- */
void tui_toast(int type, const char *title, const char *message);
void tui_toast_update(void);   /* call every tick to expire/animate */
void tui_toast_draw(void);     /* call after fb_console_flush */

/* ---- Theme integration ---- */
void tui_apply_theme(void);    /* refresh bars with current theme */

/* ---- Legacy color accessors (kept for backward compat) ---- */
brights_color_t tui_clr_black(void);
brights_color_t tui_clr_red(void);
brights_color_t tui_clr_green(void);
brights_color_t tui_clr_yellow(void);
brights_color_t tui_clr_blue(void);
brights_color_t tui_clr_magenta(void);
brights_color_t tui_clr_cyan(void);
brights_color_t tui_clr_white(void);
brights_color_t tui_clr_bright_black(void);
brights_color_t tui_clr_bright_red(void);
brights_color_t tui_clr_bright_green(void);
brights_color_t tui_clr_bright_yellow(void);
brights_color_t tui_clr_bright_blue(void);
brights_color_t tui_clr_bright_magenta(void);
brights_color_t tui_clr_bright_cyan(void);
brights_color_t tui_clr_bright_white(void);

#endif
