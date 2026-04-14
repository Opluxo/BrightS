#ifndef BRIGHTS_FB_H
#define BRIGHTS_FB_H

#include <stdint.h>

/* Framebuffer info structure */
typedef struct {
  void *framebuffer;
  uint32_t width;
  uint32_t height;
  uint32_t pitch;
  uint32_t bpp;          /* bits per pixel */
  uint32_t bytes_per_pixel;
  uint32_t red_mask;
  uint32_t green_mask;
  uint32_t blue_mask;
  uint32_t red_shift;
  uint32_t green_shift;
  uint32_t blue_shift;
  int initialized;
} brights_fb_info_t;

/* Color RGB structure */
typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
} brights_color_t;

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

#endif
