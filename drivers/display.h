#ifndef BRIGHTS_DISPLAY_H
#define BRIGHTS_DISPLAY_H

#include <stdint.h>
#include "fb.h"

#define DISPLAY_MAX_MODES 32

typedef struct {
  uint32_t width;
  uint32_t height;
  uint32_t pitch;
  uint32_t bpp;
  uint32_t format;
} display_mode_t;

typedef struct {
  display_mode_t modes[DISPLAY_MAX_MODES];
  int mode_count;
  int current_mode;
  int initialized;
} display_manager_t;

typedef enum {
  DISPLAY_ORIENTATION_NORMAL = 0,
  DISPLAY_ORIENTATION_90,
  DISPLAY_ORIENTATION_180,
  DISPLAY_ORIENTATION_270,
  DISPLAY_ORIENTATION_FLIP_H,
  DISPLAY_ORIENTATION_FLIP_V
} display_orientation_t;

typedef struct {
  int brightness;
  int contrast;
  int saturation;
  int gamma;
} display_adjustment_t;

typedef struct {
  uint32_t x;
  uint32_t y;
  uint32_t width;
  uint32_t height;
} display_region_t;

void display_init(void *gop);
int display_available(void);
display_manager_t *display_get_manager(void);

int display_set_mode(int mode_index);
int display_get_mode_count(void);
display_mode_t *display_get_mode(int index);
display_mode_t *display_get_current_mode(void);

void display_clear(brights_color_t color);
void display_flip(void);

void display_set_brightness(int level);
int display_get_brightness(void);
void display_adjustment_set(display_adjustment_t *adj);
void display_adjustment_get(display_adjustment_t *adj);

void display_set_orientation(display_orientation_t orientation);
display_orientation_t display_get_orientation(void);

void display_power_on(void);
void display_power_off(void);
int display_is_powered_on(void);

int display_mode_find(uint32_t width, uint32_t height);
int display_mode_find_closest(uint32_t width, uint32_t height);

void display_region_set(display_region_t *region);
void display_region_reset(void);
display_region_t *display_region_get(void);

void display_screenshot(void *buffer, int x, int y, int width, int height);

typedef struct {
  int x;
  int y;
  int visible;
  int width;
  int height;
  brights_color_t color;
  uint32_t blink_rate;
  uint32_t blink_state;
} display_cursor_t;

void display_cursor_init(void);
void display_cursor_show(void);
void display_cursor_hide(void);
void display_cursor_move(int x, int y);
void display_cursor_set_color(brights_color_t color);
void display_cursor_update(void);

typedef struct {
  int enabled;
  int width;
  int height;
  brights_color_t color;
  uint32_t timeout_ms;
} display_screensaver_t;

void display_screensaver_init(void);
void display_screensaver_enable(void);
void display_screensaver_disable(void);
void display_screensaver_set_timeout(uint32_t ms);
void display_screensaver_reset(void);
int display_screensaver_is_active(void);

#endif
