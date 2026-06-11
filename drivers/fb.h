#ifndef BRIGHTS_FB_H
#define BRIGHTS_FB_H

#include <stdint.h>

/* Color RGB structure */
typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
} brights_color_t;

/* Framebuffer info structure */
typedef struct {
  void *framebuffer;
  uint32_t width;
  uint32_t height;
  uint32_t pitch;
  uint32_t bpp;
  uint32_t bytes_per_pixel;
  uint32_t red_mask;
  uint32_t green_mask;
  uint32_t blue_mask;
  uint32_t red_shift;
  uint32_t green_shift;
  uint32_t blue_shift;
  int initialized;
} brights_fb_info_t;

/* Clipping region */
typedef struct {
  int x;
  int y;
  int width;
  int height;
  int enabled;
} brights_clip_t;

/* Double buffer */
typedef struct {
  void *buffer;
  uint32_t size;
  int width;
  int height;
  int pitch;
  int active;
} brights_dbuffer_t;

/* Graphics context */
typedef struct {
  brights_color_t foreground;
  brights_color_t background;
  int thickness;
  brights_clip_t clip;
  int double_buffer;
  int vsync;
} brights_gc_t;

/* Initialize framebuffer from UEFI GOP */
int brights_fb_init(void *gop);

/* Check if framebuffer is available */
int brights_fb_available(void);

/* Get framebuffer info */
brights_fb_info_t *brights_fb_get_info(void);

/* Clear screen to a color */
void brights_fb_clear(brights_color_t color);

/* Draw a single pixel */
void brights_fb_draw_pixel(int x, int y, brights_color_t color);

/* Draw a horizontal line */
void brights_fb_draw_hline(int x, int y, int width, brights_color_t color);

/* Draw a vertical line */
void brights_fb_draw_vline(int x, int y, int height, brights_color_t color);

/* Draw a rectangle outline */
void brights_fb_draw_rect(int x, int y, int width, int height, brights_color_t color);

/* Draw a filled rectangle */
void brights_fb_fill_rect(int x, int y, int width, int height, brights_color_t color);

/* Copy pixels from buffer to framebuffer */
void brights_fb_blit(void *buffer, int x, int y, int width, int height, int pitch);

/* Get pixel color at position */
brights_color_t brights_fb_get_pixel(int x, int y);

/* Fill entire screen with a color */
void brights_fb_fill(brights_color_t color);

/* Bresenham line drawing */
void brights_fb_draw_line(int x0, int y0, int x1, int y1, brights_color_t color);

/* Draw circle outline */
void brights_fb_draw_circle(int cx, int cy, int radius, brights_color_t color);

/* Draw filled circle */
void brights_fb_fill_circle(int cx, int cy, int radius, brights_color_t color);

/* Draw ellipse outline */
void brights_fb_draw_ellipse(int cx, int cy, int rx, int ry, brights_color_t color);

/* Draw filled ellipse */
void brights_fb_fill_ellipse(int cx, int cy, int rx, int ry, brights_color_t color);

/* Draw triangle */
void brights_fb_draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, brights_color_t color);

/* Draw filled triangle */
void brights_fb_fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2, brights_color_t color);

/* Rounded rectangle */
void brights_fb_draw_rounded_rect(int x, int y, int width, int height, int radius, brights_color_t color);
void brights_fb_fill_rounded_rect(int x, int y, int width, int height, int radius, brights_color_t color);

/* Alpha blending */
brights_color_t brights_color_blend(brights_color_t fg, brights_color_t bg, uint8_t alpha);
void brights_fb_draw_pixel_alpha(int x, int y, brights_color_t color, uint8_t alpha);

/* Double buffering functions */
int brights_dbuffer_init(void);
void brights_dbuffer_flip(void);
void brights_dbuffer_clear(brights_color_t color);
void brights_dbuffer_draw_pixel(int x, int y, brights_color_t color);
brights_color_t brights_dbuffer_get_pixel(int x, int y);
void brights_dbuffer_draw_line(int x0, int y0, int x1, int y1, brights_color_t color);
void brights_dbuffer_fill_rect(int x, int y, int width, int height, brights_color_t color);
void brights_dbuffer_draw_rect(int x, int y, int width, int height, brights_color_t color);
void brights_dbuffer_draw_circle(int cx, int cy, int radius, brights_color_t color);
void brights_dbuffer_fill_circle(int cx, int cy, int radius, brights_color_t color);
void brights_dbuffer_blit(void *buffer, int x, int y, int width, int height, int pitch);
void brights_dbuffer_fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2, brights_color_t color);

/* Clipping region functions */
void brights_clip_set(int x, int y, int width, int height);
void brights_clip_disable(void);
brights_clip_t *brights_clip_get(void);
int brights_clip_point(int x, int y);
int brights_clip_rect(int x, int y, int width, int height);

/* Graphics context functions */
void brights_gc_init(brights_gc_t *gc);
void brights_gc_set_fg(brights_gc_t *gc, brights_color_t color);
void brights_gc_set_bg(brights_gc_t *gc, brights_color_t color);
void brights_gc_set_thickness(brights_gc_t *gc, int thickness);
void brights_gc_set_clip(brights_gc_t *gc, int x, int y, int width, int height);
void brights_gc_enable_double_buffer(brights_gc_t *gc);
void brights_gc_disable_double_buffer(brights_gc_t *gc);

/* Utility functions */
brights_color_t brights_rgb(uint8_t r, uint8_t g, uint8_t b);
brights_color_t brights_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
brights_color_t brights_color_from_hex(uint32_t hex);
uint32_t brights_color_to_hex(brights_color_t color);
brights_color_t brights_color_lighten(brights_color_t color, float factor);
brights_color_t brights_color_darken(brights_color_t color, float factor);
int brights_color_equal(brights_color_t c1, brights_color_t c2);

/* Anti-aliased drawing */
void brights_fb_draw_line_aa(int x0, int y0, int x1, int y1, brights_color_t color);

/* Gradient fill */
void brights_fb_fill_gradient_h(int x, int y, int width, int height, brights_color_t c1, brights_color_t c2);
void brights_fb_fill_gradient_v(int x, int y, int width, int height, brights_color_t c1, brights_color_t c2);

/* Scroll functions */
void brights_fb_scroll(int dx, int dy);
void brights_fb_scroll_area(int x, int y, int width, int height, int dx, int dy);

#endif
