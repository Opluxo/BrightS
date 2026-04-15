#ifndef BRIGHTS_TTY_H
#define BRIGHTS_TTY_H

#include <stdint.h>
#include "fb.h"

/* TTY modes */
#define TTY_MODE_RAW    0
#define TTY_MODE_COOKED 1

/* Console output modes */
#define TTY_OUTPUT_SERIAL  0
#define TTY_OUTPUT_FRAMEBUFFER 1
#define TTY_OUTPUT_BOTH    2

void brights_tty_init(void);
void brights_tty_write(const uint16_t *s);
void brights_tty_write_str(const char *s);
void brights_tty_write_char(char ch);
int brights_tty_read_char(char *out_ch);
char brights_tty_read_char_blocking(void);

/* Set/get TTY mode */
void brights_tty_set_mode(int mode);
int brights_tty_get_mode(void);

/* Framebuffer console functions */
typedef struct {
  int x;
  int y;
  int width;
  int height;
  int cursor_x;
  int cursor_y;
  int tab_width;
  brights_color_t fg_color;
  brights_color_t bg_color;
  int scroll_enabled;
  int cursor_visible;
} fb_console_t;

void fb_console_init(void);
void fb_console_clear(void);
void fb_console_set_colors(brights_color_t fg, brights_color_t bg);
void fb_console_goto(int x, int y);
void fb_console_get_pos(int *x, int *y);
void fb_console_put_char(char c);
void fb_console_write_str(const char *s);
void fb_console_write_line(const char *s);
void fb_console_newline(void);
void fb_console_scroll(void);
void fb_console_set_cursor_visible(int visible);
void fb_console_update_cursor(void);
fb_console_t *fb_console_get_info(void);

/* printf-style functions for framebuffer console */
void fb_printf(const char *fmt, ...);
void fb_printf_color(brights_color_t fg, brights_color_t bg, const char *fmt, ...);

#endif
