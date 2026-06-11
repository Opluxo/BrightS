#include "display.h"
#include "fb.h"
#include "../include/kernel/uefi.h"

static display_manager_t dm = {0};
static display_orientation_t current_orientation = DISPLAY_ORIENTATION_NORMAL;
static display_adjustment_t current_adjustment = {100, 100, 100, 100};
static display_cursor_t cursor = {0, 0, 1, 16, 16, {255, 255, 255, 255}, 500, 0};
static display_screensaver_t screensaver = {0, 0, 0, {0, 0, 0, 255}, 300000};
static display_region_t active_region = {0, 0, 0, 0};
static int powered_on = 1;
static uint32_t last_activity_time = 0;
static void *gop_interface = 0;

void display_init(void *gop)
{
  if (!gop) return;
  
  gop_interface = gop;
  EFI_GRAPHICS_OUTPUT_PROTOCOL *gop_proto = (EFI_GRAPHICS_OUTPUT_PROTOCOL *)gop;
  if (!gop_proto || !gop_proto->Mode) return;
  
  dm.mode_count = 0;
  dm.current_mode = -1;
  
  for (uint32_t i = 0; i < gop_proto->Mode->MaxMode && dm.mode_count < DISPLAY_MAX_MODES; i++) {
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info = 0;
    uint64_t size_of_info = 0;
    
    if (gop_proto->QueryMode(gop_proto, i, &size_of_info, &info) == EFI_SUCCESS && info) {
      dm.modes[dm.mode_count].width = info->HorizontalResolution;
      dm.modes[dm.mode_count].height = info->VerticalResolution;
      dm.modes[dm.mode_count].pitch = info->PixelsPerScanLine * 4;
      dm.modes[dm.mode_count].bpp = 32;
      dm.modes[dm.mode_count].format = info->PixelFormat;
      dm.mode_count++;
    }
  }
  
  if (dm.mode_count > 0) {
    dm.current_mode = gop_proto->Mode->Mode;
    dm.initialized = 1;
  }
  
  brights_fb_info_t *fb = brights_fb_get_info();
  active_region.x = 0;
  active_region.y = 0;
  if (fb) {
    active_region.width = fb->width;
    active_region.height = fb->height;
  }
}

int display_available(void) { return dm.initialized; }
display_manager_t *display_get_manager(void) { return &dm; }

int display_set_mode(int mode_index)
{
  if (!dm.initialized || mode_index < 0 || mode_index >= dm.mode_count) return -1;
  
  EFI_GRAPHICS_OUTPUT_PROTOCOL *gop_proto = (EFI_GRAPHICS_OUTPUT_PROTOCOL *)gop_interface;
  if (!gop_proto) return -1;
  
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info = 0;
  uint64_t size_of_info = 0;
  
  if (gop_proto->QueryMode(gop_proto, mode_index, &size_of_info, &info) != EFI_SUCCESS) return -1;
  if (gop_proto->SetMode(gop_proto, mode_index) != EFI_SUCCESS) return -1;
  
  dm.current_mode = mode_index;
  brights_fb_init(gop_interface);
  
  active_region.x = 0;
  active_region.y = 0;
  active_region.width = dm.modes[mode_index].width;
  active_region.height = dm.modes[mode_index].height;
  
  return 0;
}

int display_get_mode_count(void) { return dm.mode_count; }
display_mode_t *display_get_mode(int index) { return (index >= 0 && index < dm.mode_count) ? &dm.modes[index] : 0; }
display_mode_t *display_get_current_mode(void) { return (dm.current_mode >= 0 && dm.current_mode < dm.mode_count) ? &dm.modes[dm.current_mode] : 0; }

void display_clear(brights_color_t color) { brights_fb_clear(color); }
void display_flip(void) { brights_dbuffer_flip(); }

void display_set_brightness(int level)
{
  if (level < 0) level = 0;
  else if (level > 100) level = 100;
  current_adjustment.brightness = level;
}

int display_get_brightness(void) { return current_adjustment.brightness; }

void display_adjustment_set(display_adjustment_t *adj)
{
  if (!adj) return;
  if (adj->brightness <= 100) current_adjustment.brightness = adj->brightness;
  if (adj->contrast <= 100) current_adjustment.contrast = adj->contrast;
  if (adj->saturation <= 100) current_adjustment.saturation = adj->saturation;
  if (adj->gamma >= 1 && adj->gamma <= 500) current_adjustment.gamma = adj->gamma;
}

void display_adjustment_get(display_adjustment_t *adj) { if (adj) *adj = current_adjustment; }
void display_set_orientation(display_orientation_t orientation) { current_orientation = orientation; }
display_orientation_t display_get_orientation(void) { return current_orientation; }
void display_power_on(void) { powered_on = 1; display_clear(brights_rgb(0, 0, 0)); }
void display_power_off(void) { powered_on = 0; }
int display_is_powered_on(void) { return powered_on; }

int display_mode_find(uint32_t width, uint32_t height)
{
  for (int i = 0; i < dm.mode_count; i++) {
    if (dm.modes[i].width == width && dm.modes[i].height == height) return i;
  }
  return -1;
}

int display_mode_find_closest(uint32_t width, uint32_t height)
{
  int best = -1;
  uint32_t best_diff = 0xFFFFFFFF;
  
  for (int i = 0; i < dm.mode_count; i++) {
    uint32_t w_diff = (dm.modes[i].width > width) ? dm.modes[i].width - width : width - dm.modes[i].width;
    uint32_t h_diff = (dm.modes[i].height > height) ? dm.modes[i].height - height : height - dm.modes[i].height;
    uint32_t diff = w_diff + h_diff;
    
    if (diff < best_diff) {
      best_diff = diff;
      best = i;
    }
  }
  return best;
}

void display_region_set(display_region_t *region) { if (region) active_region = *region; }

void display_region_reset(void)
{
  brights_fb_info_t *fb = brights_fb_get_info();
  active_region.x = 0;
  active_region.y = 0;
  if (fb) {
    active_region.width = fb->width;
    active_region.height = fb->height;
  }
}

display_region_t *display_region_get(void) { return &active_region; }

void display_screenshot(void *buffer, int x, int y, int width, int height)
{
  if (!buffer || x < 0 || y < 0) return;
  
  brights_fb_info_t *fb = brights_fb_get_info();
  if (!fb) return;
  
  if (x + width > (int)fb->width) width = fb->width - x;
  if (y + height > (int)fb->height) height = fb->height - y;
  
  brights_fb_blit(buffer, x, y, width, height, fb->pitch);
}

void display_cursor_init(void) { cursor.visible = 1; cursor.blink_state = 1; }
void display_cursor_show(void) { cursor.visible = 1; cursor.blink_state = 1; }
void display_cursor_hide(void) { cursor.visible = 0; }
void display_cursor_move(int x, int y) { cursor.x = x; cursor.y = y; }
void display_cursor_set_color(brights_color_t color) { cursor.color = color; }
void display_cursor_update(void) { if (cursor.visible && cursor.blink_state) brights_fb_fill_rect(cursor.x, cursor.y, cursor.width, cursor.height, cursor.color); }

void display_screensaver_init(void) { screensaver.enabled = 0; screensaver.timeout_ms = 300000; }
void display_screensaver_enable(void) { screensaver.enabled = 1; }
void display_screensaver_disable(void) { screensaver.enabled = 0; }
void display_screensaver_set_timeout(uint32_t ms) { screensaver.timeout_ms = ms; }
void display_screensaver_reset(void) { last_activity_time = 0; }
int display_screensaver_is_active(void) { return screensaver.enabled; }
