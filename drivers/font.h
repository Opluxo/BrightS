#ifndef BRIGHTS_FONT_H
#define BRIGHTS_FONT_H

#include <stdint.h>

#define FONT_WIDTH 16
#define FONT_HEIGHT 16

typedef struct {
    uint32_t codepoint;
    uint8_t bitmap[32];
} chinese_glyph_t;

extern const uint8_t brights_font_8x16[256][16];
extern const chinese_glyph_t chinese_glyphs[];
extern const int chinese_glyph_count;

uint32_t brights_utf8_to_codepoint(const char *str, int *len);
const uint8_t* brights_get_chinese_glyph(uint32_t codepoint);

void brights_font_draw_char(int x, int y, char c, uint32_t fg_color, uint32_t bg_color);

void brights_font_draw_string(int x, int y, const char *str, uint32_t fg_color, uint32_t bg_color);

int brights_font_string_width(const char *str);

int brights_font_char_width(char c);

#endif
