#ifndef BRIGHTS_TUI_H
#define BRIGHTS_TUI_H

#include <stdint.h>
#include "fb.h"

#define TUI_TITLE_HEIGHT 1
#define TUI_STATUSBAR_HEIGHT 1

void tui_init(void);
void tui_draw_title_bar(const char *left, const char *right);
void tui_draw_status_bar(const char *left, const char *right);
void tui_refresh_status(const char *user, int is_root);
void tui_clear(void);

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
