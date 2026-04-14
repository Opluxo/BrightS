#ifndef BRIGHTS_FONT_H
#define BRIGHTS_FONT_H

#include <stdint.h>

#define FONT_WIDTH 8
#define FONT_HEIGHT 16

extern const uint8_t brights_font_8x16[256][16];

void brights_font_draw_char(int x, int y, char c, uint32_t fg_color, uint32_t bg_color);

void brights_font_draw_string(int x, int y, const char *str, uint32_t fg_color, uint32_t bg_color);

int brights_font_string_width(const char *str);

int brights_font_char_width(char c);

#endif
