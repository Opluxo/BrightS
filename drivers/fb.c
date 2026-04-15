#include "fb.h"
#include "../include/kernel/uefi.h"
#include "../include/kernel/stddef.h"
#include <stdint.h>

static brights_fb_info_t fb_info = {0};
static brights_clip_t global_clip = {0, 0, 0, 0, 0};
static brights_dbuffer_t dbuffer = {0};

static inline uint32_t color_to_pixel_fast(brights_color_t color)
{
  return (color.r << fb_info.red_shift) |
         (color.g << fb_info.green_shift) |
         (color.b << fb_info.blue_shift);
}

static inline uint32_t pixel_at_fast(int x, int y)
{
  uint32_t *fb = (uint32_t *)fb_info.framebuffer;
  return fb[y * (fb_info.pitch / 4) + x];
}

static inline void set_pixel_fast(int x, int y, uint32_t pixel)
{
  uint32_t *fb = (uint32_t *)fb_info.framebuffer;
  fb[y * (fb_info.pitch / 4) + x] = pixel;
}

static inline int apply_clip_rect(int *x, int *y, int *w, int *h)
{
  if (!global_clip.enabled) return 1;
  
  int cx2 = global_clip.x + global_clip.width;
  int cy2 = global_clip.y + global_clip.height;
  int tx2 = *x + *w;
  int ty2 = *y + *h;
  
  if (tx2 <= global_clip.x || ty2 <= global_clip.y || 
      *x >= cx2 || *y >= cy2) return 0;
  
  if (*x < global_clip.x) { *w -= (global_clip.x - *x); *x = global_clip.x; }
  if (*y < global_clip.y) { *h -= (global_clip.y - *y); *y = global_clip.y; }
  if (tx2 > cx2) *w -= (tx2 - cx2);
  if (ty2 > cy2) *h -= (ty2 - cy2);
  
  return (*w > 0 && *h > 0);
}

int brights_fb_init(void *gop_ptr)
{
  if (!gop_ptr) return -1;
  
  EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = (EFI_GRAPHICS_OUTPUT_PROTOCOL *)gop_ptr;
  if (!gop || !gop->Mode) return -1;
  
  EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *mode = gop->Mode;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info = mode->Info;
  
  fb_info.framebuffer = (void *)(uintptr_t)mode->FrameBufferBase;
  fb_info.width = info->HorizontalResolution;
  fb_info.height = info->VerticalResolution;
  fb_info.pitch = info->PixelsPerScanLine * 4;
  
  switch (info->PixelFormat) {
    case PixelRedGreenBlueReserved8BitPerColor:
    case PixelBlueGreenRedReserved8BitPerColor:
      fb_info.bpp = 32;
      fb_info.bytes_per_pixel = 4;
      fb_info.red_shift = 16;
      fb_info.green_shift = 8;
      fb_info.blue_shift = 0;
      break;
    case PixelBitMask:
      fb_info.red_shift = __builtin_ctz(info->PixelInformation.RedMask);
      fb_info.green_shift = __builtin_ctz(info->PixelInformation.GreenMask);
      fb_info.blue_shift = __builtin_ctz(info->PixelInformation.BlueMask);
      fb_info.bpp = 32;
      fb_info.bytes_per_pixel = 4;
      break;
    default:
      fb_info.bpp = 32;
      fb_info.bytes_per_pixel = 4;
      fb_info.red_shift = 16;
      fb_info.green_shift = 8;
      fb_info.blue_shift = 0;
      break;
  }
  
  fb_info.initialized = 1;
  
  if (dbuffer.buffer) brights_dbuffer_init();
  
  return 0;
}

int brights_fb_available(void) { return fb_info.initialized; }
brights_fb_info_t *brights_fb_get_info(void) { return fb_info.initialized ? &fb_info : 0; }

void brights_fb_draw_pixel(int x, int y, brights_color_t color)
{
  if (!fb_info.initialized) return;
  if ((uint32_t)x >= fb_info.width || (uint32_t)y >= fb_info.height) return;
  if (global_clip.enabled && !brights_clip_point(x, y)) return;
  set_pixel_fast(x, y, color_to_pixel_fast(color));
}

void brights_fb_draw_hline(int x, int y, int width, brights_color_t color)
{
  if (!fb_info.initialized) return;
  if ((uint32_t)y >= fb_info.height || width <= 0) return;
  
  int cx = x, cw = width;
  if (cx < 0) { cw += cx; cx = 0; }
  int max_w = fb_info.width - cx;
  if (cw > max_w) cw = max_w;
  if (cw <= 0) return;
  
  if (global_clip.enabled) {
    int tmp_x = cx, tmp_y = y, tmp_w = cw, tmp_h = 1;
    if (!apply_clip_rect(&tmp_x, &tmp_y, &tmp_w, &tmp_h)) return;
    cx = tmp_x; cw = tmp_w;
  }
  
  uint32_t *fb = (uint32_t *)fb_info.framebuffer;
  uint32_t pixel = color_to_pixel_fast(color);
  uint32_t *row = fb + y * (fb_info.pitch / 4) + cx;
  
  for (int i = 0; i < cw; i++) row[i] = pixel;
}

void brights_fb_draw_vline(int x, int y, int height, brights_color_t color)
{
  if (!fb_info.initialized) return;
  if ((uint32_t)x >= fb_info.width || height <= 0) return;
  
  int cy = y, ch = height;
  if (cy < 0) { ch += cy; cy = 0; }
  int max_h = fb_info.height - cy;
  if (ch > max_h) ch = max_h;
  if (ch <= 0) return;
  
  if (global_clip.enabled) {
    int tmp_x = x, tmp_y = cy, tmp_w = 1, tmp_h = ch;
    if (!apply_clip_rect(&tmp_x, &tmp_y, &tmp_w, &tmp_h)) return;
    cy = tmp_y; ch = tmp_h;
  }
  
  uint32_t *fb = (uint32_t *)fb_info.framebuffer;
  uint32_t pixel = color_to_pixel_fast(color);
  uint32_t stride = fb_info.pitch / 4;
  uint32_t offset = cy * stride + x;
  
  for (int i = 0; i < ch; i++) fb[offset + i * stride] = pixel;
}

void brights_fb_draw_rect(int x, int y, int width, int height, brights_color_t color)
{
  if (width <= 0 || height <= 0) return;
  brights_fb_draw_hline(x, y, width, color);
  if (height > 1) brights_fb_draw_hline(x, y + height - 1, width, color);
  if (height > 2) {
    brights_fb_draw_vline(x, y + 1, height - 2, color);
    brights_fb_draw_vline(x + width - 1, y + 1, height - 2, color);
  }
}

void brights_fb_fill_rect(int x, int y, int width, int height, brights_color_t color)
{
  if (!fb_info.initialized || width <= 0 || height <= 0) return;
  
  int cx = x, cy = y, cw = width, ch = height;
  if (!apply_clip_rect(&cx, &cy, &cw, &ch)) return;
  
  uint32_t *fb = (uint32_t *)fb_info.framebuffer;
  uint32_t pixel = color_to_pixel_fast(color);
  uint32_t stride = fb_info.pitch / 4;
  uint32_t *row = fb + cy * stride + cx;
  
  if (cw == (int)fb_info.width && stride == (uint32_t)cw) {
    uint32_t total = (uint32_t)cw * ch;
    for (uint32_t i = 0; i < total; i++) row[i] = pixel;
  } else {
    for (int i = 0; i < ch; i++) {
      uint32_t *p = row + i * stride;
      for (int j = 0; j < cw; j++) p[j] = pixel;
    }
  }
}

void brights_fb_blit(void *buffer, int x, int y, int width, int height, int pitch)
{
  if (!fb_info.initialized || !buffer) return;
  if ((uint32_t)x >= fb_info.width || (uint32_t)y >= fb_info.height) return;
  
  int cx = x, cy = y, cw = width, ch = height;
  if (!apply_clip_rect(&cx, &cy, &cw, &ch)) return;
  
  uint32_t *fb = (uint32_t *)fb_info.framebuffer;
  uint32_t *src = (uint32_t *)buffer;
  uint32_t fb_stride = fb_info.pitch / 4;
  uint32_t src_stride = (uint32_t)pitch / 4;
  int row_offset = cx - x;
  
  for (int i = 0; i < ch; i++) {
    uint32_t *dst_row = fb + (cy + i) * fb_stride + cx;
    uint32_t *src_row = src + i * src_stride + row_offset;
    for (int j = 0; j < cw; j++) dst_row[j] = src_row[j];
  }
}

brights_color_t brights_fb_get_pixel(int x, int y)
{
  brights_color_t color = {0, 0, 0, 255};
  if (!fb_info.initialized) return color;
  if ((uint32_t)x >= fb_info.width || (uint32_t)y >= fb_info.height) return color;
  
  uint32_t pixel = pixel_at_fast(x, y);
  color.r = (pixel >> fb_info.red_shift) & 0xFF;
  color.g = (pixel >> fb_info.green_shift) & 0xFF;
  color.b = (pixel >> fb_info.blue_shift) & 0xFF;
  return color;
}

void brights_fb_fill(brights_color_t color)
{
  brights_fb_fill_rect(0, 0, fb_info.width, fb_info.height, color);
}

void brights_fb_clear(brights_color_t color) { brights_fb_fill(color); }

void brights_fb_draw_line(int x0, int y0, int x1, int y1, brights_color_t color)
{
  int dx = x1 - x0, dy = y1 - y0;
  int abs_dx = dx >= 0 ? dx : -dx;
  int abs_dy = dy >= 0 ? dy : -dy;
  int xinc = dx > 0 ? 1 : (dx < 0 ? -1 : 0);
  int yinc = dy > 0 ? 1 : (dy < 0 ? -1 : 0);
  uint32_t pixel = color_to_pixel_fast(color);
  
  if (abs_dx > abs_dy) {
    int e = abs_dx / 2;
    for (int i = 0; i <= abs_dx; i++) {
      set_pixel_fast(x0, y0, pixel);
      x0 += xinc; e -= abs_dy;
      if (e < 0) { y0 += yinc; e += abs_dx; }
    }
  } else if (abs_dy > 0) {
    int e = abs_dy / 2;
    for (int i = 0; i <= abs_dy; i++) {
      set_pixel_fast(x0, y0, pixel);
      y0 += yinc; e -= abs_dx;
      if (e < 0) { x0 += xinc; e += abs_dy; }
    }
  } else {
    set_pixel_fast(x0, y0, pixel);
  }
}

void brights_fb_draw_circle(int cx, int cy, int radius, brights_color_t color)
{
  if (radius <= 0) return;
  
  uint32_t pixel = color_to_pixel_fast(color);
  int x = 0, y = radius;
  int d = 3 - 2 * radius;
  
  while (x <= y) {
    set_pixel_fast(cx + x, cy + y, pixel);
    set_pixel_fast(cx - x, cy + y, pixel);
    set_pixel_fast(cx + x, cy - y, pixel);
    set_pixel_fast(cx - x, cy - y, pixel);
    set_pixel_fast(cx + y, cy + x, pixel);
    set_pixel_fast(cx - y, cy + x, pixel);
    set_pixel_fast(cx + y, cy - x, pixel);
    set_pixel_fast(cx - y, cy - x, pixel);
    
    if (d < 0) d += 4 * x++ + 6;
    else d += 4 * (x++ - y--) + 10;
  }
}

void brights_fb_fill_circle(int cx, int cy, int radius, brights_color_t color)
{
  if (radius <= 0) return;
  
  int x = 0, y = radius;
  int d = 3 - 2 * radius;
  
  while (x <= y) {
    brights_fb_draw_hline(cx - x, cy + y, 2 * x + 1, color);
    if (y != x) brights_fb_draw_hline(cx - x, cy - y, 2 * x + 1, color);
    brights_fb_draw_hline(cx - y, cy + x, 2 * y + 1, color);
    if (y != x) brights_fb_draw_hline(cx - y, cy - x, 2 * y + 1, color);
    
    if (d < 0) d += 4 * x++ + 6;
    else d += 4 * (x++ - y--) + 10;
  }
}

static inline void plot_ellipse_points(int cx, int cy, int x, int y, uint32_t pixel)
{
  set_pixel_fast(cx + x, cy + y, pixel);
  set_pixel_fast(cx - x, cy + y, pixel);
  set_pixel_fast(cx + x, cy - y, pixel);
  set_pixel_fast(cx - x, cy - y, pixel);
}

void brights_fb_draw_ellipse(int cx, int cy, int rx, int ry, brights_color_t color)
{
  if (rx <= 0 || ry <= 0) return;
  
  uint32_t pixel = color_to_pixel_fast(color);
  int rx2 = rx * rx, ry2 = ry * ry;
  int two_rx2 = 2 * rx2, two_ry2 = 2 * ry2;
  
  int x = 0, y = ry;
  int px = 0, py = two_rx2 * y;
  
  plot_ellipse_points(cx, cy, x, y, pixel);
  
  int p = ry2 - rx2 * ry + (rx2 >> 2);
  while (px < py) {
    x++; px += two_ry2;
    if (p < 0) p += ry2 + px;
    else { y--; py -= two_rx2; p += ry2 + px - py; }
    plot_ellipse_points(cx, cy, x, y, pixel);
  }
  
  p = (int)((ry2 * (x + 1) * (x + 1) + rx2 * (y - 1) * (y - 1) - (uint32_t)rx2 * ry2) >> 0);
  while (y > 0) {
    y--; py -= two_rx2;
    if (p > 0) p += rx2 - py;
    else { x++; px += two_ry2; p += rx2 - py + px; }
    plot_ellipse_points(cx, cy, x, y, pixel);
  }
}

void brights_fb_fill_ellipse(int cx, int cy, int rx, int ry, brights_color_t color)
{
  if (rx <= 0 || ry <= 0) return;
  
  int rx2 = rx * rx, ry2 = ry * ry;
  int two_rx2 = 2 * rx2, two_ry2 = 2 * ry2;
  
  int x = 0, y = ry;
  int px = 0, py = two_rx2 * y;
  
  brights_fb_draw_hline(cx, cy + y, 1, color);
  brights_fb_draw_hline(cx, cy - y, 1, color);
  
  int p = ry2 - rx2 * ry + (rx2 >> 2);
  while (px < py) {
    x++; px += two_ry2;
    if (p < 0) p += ry2 + px;
    else { y--; py -= two_rx2; p += ry2 + px - py; }
    brights_fb_draw_hline(cx - x, cy + y, 2 * x + 1, color);
    brights_fb_draw_hline(cx - x, cy - y, 2 * x + 1, color);
  }
  
  p = (int)((ry2 * (x + 1) * (x + 1) + rx2 * (y - 1) * (y - 1) - (uint32_t)rx2 * ry2) >> 0);
  while (y > 0) {
    y--; py -= two_rx2;
    if (p > 0) p += rx2 - py;
    else { x++; px += two_ry2; p += rx2 - py + px; }
    brights_fb_draw_hline(cx - x, cy + y, 2 * x + 1, color);
    brights_fb_draw_hline(cx - x, cy - y, 2 * x + 1, color);
  }
  
  brights_fb_draw_hline(cx - x, cy, 2 * x + 1, color);
}

void brights_fb_draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, brights_color_t color)
{
  brights_fb_draw_line(x0, y0, x1, y1, color);
  brights_fb_draw_line(x1, y1, x2, y2, color);
  brights_fb_draw_line(x2, y2, x0, y0, color);
}

void brights_fb_fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2, brights_color_t color)
{
  if (y0 > y1) { int t = y0; y0 = y1; y1 = t; t = x0; x0 = x1; x1 = t; }
  if (y0 > y2) { int t = y0; y0 = y2; y2 = t; t = x0; x0 = x2; x2 = t; }
  if (y1 > y2) { int t = y1; y1 = y2; y2 = t; t = x1; x1 = x2; x2 = t; }
  
  int total_height = y2 - y0;
  if (total_height <= 0) return;
  
  for (int y = y0; y <= y2; y++) {
    float seg_start, seg_end;
    
    if (y < y1) {
      float t1 = (float)(y - y0) / (y1 - y0);
      float t2 = (float)(y - y0) / (y2 - y0);
      seg_start = x0 + t1 * (x1 - x0);
      seg_end = x0 + t2 * (x2 - x0);
    } else {
      float t1 = (float)(y - y1) / (y2 - y1);
      float t2 = (float)(y - y0) / (y2 - y0);
      seg_start = x1 + t1 * (x2 - x1);
      seg_end = x0 + t2 * (x2 - x0);
    }
    
    int xs = (int)(seg_start < seg_end ? seg_start : seg_end);
    int xe = (int)(seg_start < seg_end ? seg_end : seg_start);
    
    if (xs < 0) xs = 0;
    int max_x = (int)fb_info.width;
    if (xe > max_x) xe = max_x;
    
    if (xe > xs) brights_fb_draw_hline(xs, y, xe - xs, color);
  }
}

void brights_fb_draw_rounded_rect(int x, int y, int width, int height, int radius, brights_color_t color)
{
  if (radius <= 0) { brights_fb_draw_rect(x, y, width, height, color); return; }
  
  if (radius > width / 2) radius = width / 2;
  if (radius > height / 2) radius = height / 2;
  
  brights_fb_draw_hline(x + radius, y, width - 2 * radius, color);
  brights_fb_draw_hline(x + radius, y + height - 1, width - 2 * radius, color);
  if (height > 2 * radius) {
    brights_fb_draw_vline(x, y + radius, height - 2 * radius, color);
    brights_fb_draw_vline(x + width - 1, y + radius, height - 2 * radius, color);
  }
  
  uint32_t pixel = color_to_pixel_fast(color);
  int cx1 = x + radius, cy1 = y + radius;
  int cx2 = x + width - radius - 1, cy2 = y + height - radius - 1;
  int x0 = 0, y0 = radius, d = 3 - 2 * radius;
  
  while (x0 <= y0) {
    set_pixel_fast(cx1 + y0, cy1 - x0, pixel);
    set_pixel_fast(cx1 + x0, cy1 - y0, pixel);
    set_pixel_fast(cx1 + x0, cy2 + y0, pixel);
    set_pixel_fast(cx1 + y0, cy2 + x0, pixel);
    if (d < 0) d += 4 * x0++ + 6;
    else d += 4 * (x0++ - y0--) + 10;
  }
  
  x0 = 0, y0 = radius, d = 3 - 2 * radius;
  while (x0 <= y0) {
    set_pixel_fast(cx2 - y0, cy1 - x0, pixel);
    set_pixel_fast(cx2 - x0, cy1 - y0, pixel);
    set_pixel_fast(cx2 - x0, cy2 + y0, pixel);
    set_pixel_fast(cx2 - y0, cy2 + x0, pixel);
    if (d < 0) d += 4 * x0++ + 6;
    else d += 4 * (x0++ - y0--) + 10;
  }
}

void brights_fb_fill_rounded_rect(int x, int y, int width, int height, int radius, brights_color_t color)
{
  if (radius <= 0) { brights_fb_fill_rect(x, y, width, height, color); return; }
  
  if (radius > width / 2) radius = width / 2;
  if (radius > height / 2) radius = height / 2;
  
  brights_fb_fill_rect(x, y + radius, width, height - 2 * radius, color);
  brights_fb_fill_rect(x + radius, y, width - 2 * radius, height, color);
  
  uint32_t pixel = color_to_pixel_fast(color);
  int cx1 = x + radius, cy1 = y + radius;
  int cx2 = x + width - radius - 1, cy2 = y + height - radius - 1;
  int x0 = 0, y0 = radius, d = 3 - 2 * radius;
  
  while (x0 <= y0) {
    brights_fb_draw_hline(cx1 - y0, cy1 - x0, y0, color);
    brights_fb_draw_hline(cx1 - y0, cy2 + x0, y0, color);
    brights_fb_draw_hline(cx2 + 1, cy1 - x0, y0, color);
    brights_fb_draw_hline(cx2 + 1, cy2 + x0, y0, color);
    if (d < 0) d += 4 * x0++ + 6;
    else d += 4 * (x0++ - y0--) + 10;
  }
}

brights_color_t brights_color_blend(brights_color_t fg, brights_color_t bg, uint8_t alpha)
{
  uint32_t inv = 255 - alpha;
  brights_color_t result;
  result.r = (uint8_t)((fg.r * alpha + bg.r * inv + 127) / 255);
  result.g = (uint8_t)((fg.g * alpha + bg.g * inv + 127) / 255);
  result.b = (uint8_t)((fg.b * alpha + bg.b * inv + 127) / 255);
  result.a = 255;
  return result;
}

void brights_fb_draw_pixel_alpha(int x, int y, brights_color_t color, uint8_t alpha)
{
  if (!fb_info.initialized) return;
  if ((uint32_t)x >= fb_info.width || (uint32_t)y >= fb_info.height) return;
  if (global_clip.enabled && !brights_clip_point(x, y)) return;
  
  brights_color_t bg = brights_fb_get_pixel(x, y);
  brights_color_t blended = brights_color_blend(color, bg, alpha);
  brights_fb_draw_pixel(x, y, blended);
}

int brights_dbuffer_init(void)
{
  if (!fb_info.initialized) return -1;
  
  uint32_t size = fb_info.pitch * fb_info.height;
  
  if (!dbuffer.buffer) {
    extern void *brights_kmalloc(size_t size);
    dbuffer.buffer = brights_kmalloc(size);
  }
  
  if (!dbuffer.buffer) return -1;
  
  dbuffer.size = size;
  dbuffer.width = fb_info.width;
  dbuffer.height = fb_info.height;
  dbuffer.pitch = fb_info.pitch;
  dbuffer.active = 1;
  
  return 0;
}

static inline void dbuffer_set_pixel_fast(int x, int y, uint32_t pixel)
{
  uint32_t *buf = (uint32_t *)dbuffer.buffer;
  buf[y * (dbuffer.pitch / 4) + x] = pixel;
}

static inline uint32_t dbuffer_pixel_at_fast(int x, int y)
{
  uint32_t *buf = (uint32_t *)dbuffer.buffer;
  return buf[y * (dbuffer.pitch / 4) + x];
}

void brights_dbuffer_flip(void)
{
  if (!dbuffer.active || !dbuffer.buffer) return;
  
  uint32_t *src = (uint32_t *)dbuffer.buffer;
  uint32_t *dst = (uint32_t *)fb_info.framebuffer;
  uint32_t pixels = (dbuffer.pitch / 4) * dbuffer.height;
  
  for (uint32_t i = 0; i < pixels; i++) dst[i] = src[i];
}

void brights_dbuffer_clear(brights_color_t color)
{
  if (!dbuffer.active) { brights_fb_clear(color); return; }
  
  uint32_t pixel = color_to_pixel_fast(color);
  uint32_t *buf = (uint32_t *)dbuffer.buffer;
  uint32_t pixels = (dbuffer.pitch / 4) * dbuffer.height;
  
  for (uint32_t i = 0; i < pixels; i++) buf[i] = pixel;
}

void brights_dbuffer_draw_pixel(int x, int y, brights_color_t color)
{
  if (!dbuffer.active) { brights_fb_draw_pixel(x, y, color); return; }
  if ((uint32_t)x >= (uint32_t)dbuffer.width || (uint32_t)y >= (uint32_t)dbuffer.height) return;
  if (global_clip.enabled && !brights_clip_point(x, y)) return;
  dbuffer_set_pixel_fast(x, y, color_to_pixel_fast(color));
}

brights_color_t brights_dbuffer_get_pixel(int x, int y)
{
  if (!dbuffer.active) return brights_fb_get_pixel(x, y);
  
  brights_color_t color = {0, 0, 0, 255};
  if ((uint32_t)x >= (uint32_t)dbuffer.width || (uint32_t)y >= (uint32_t)dbuffer.height) return color;
  
  uint32_t pixel = dbuffer_pixel_at_fast(x, y);
  color.r = (pixel >> fb_info.red_shift) & 0xFF;
  color.g = (pixel >> fb_info.green_shift) & 0xFF;
  color.b = (pixel >> fb_info.blue_shift) & 0xFF;
  return color;
}

void brights_dbuffer_draw_line(int x0, int y0, int x1, int y1, brights_color_t color)
{
  int dx = x1 - x0, dy = y1 - y0;
  int abs_dx = dx >= 0 ? dx : -dx;
  int abs_dy = dy >= 0 ? dy : -dy;
  int xinc = dx > 0 ? 1 : (dx < 0 ? -1 : 0);
  int yinc = dy > 0 ? 1 : (dy < 0 ? -1 : 0);
  uint32_t pixel = color_to_pixel_fast(color);
  
  if (abs_dx > abs_dy) {
    int e = abs_dx / 2;
    for (int i = 0; i <= abs_dx; i++) {
      dbuffer_set_pixel_fast(x0, y0, pixel);
      x0 += xinc; e -= abs_dy;
      if (e < 0) { y0 += yinc; e += abs_dx; }
    }
  } else if (abs_dy > 0) {
    int e = abs_dy / 2;
    for (int i = 0; i <= abs_dy; i++) {
      dbuffer_set_pixel_fast(x0, y0, pixel);
      y0 += yinc; e -= abs_dx;
      if (e < 0) { x0 += xinc; e += abs_dy; }
    }
  } else {
    dbuffer_set_pixel_fast(x0, y0, pixel);
  }
}

void brights_dbuffer_fill_rect(int x, int y, int width, int height, brights_color_t color)
{
  if (!dbuffer.active) { brights_fb_fill_rect(x, y, width, height, color); return; }
  if (width <= 0 || height <= 0) return;
  
  int cx = x, cy = y, cw = width, ch = height;
  if (!apply_clip_rect(&cx, &cy, &cw, &ch)) return;
  
  uint32_t pixel = color_to_pixel_fast(color);
  uint32_t stride = dbuffer.pitch / 4;
  uint32_t *buf = (uint32_t *)dbuffer.buffer;
  
  for (int i = 0; i < ch; i++) {
    uint32_t *row = buf + (cy + i) * stride + cx;
    for (int j = 0; j < cw; j++) row[j] = pixel;
  }
}

void brights_dbuffer_draw_rect(int x, int y, int width, int height, brights_color_t color)
{
  if (width <= 0 || height <= 0) return;
  brights_dbuffer_draw_line(x, y, x + width - 1, y, color);
  brights_dbuffer_draw_line(x, y + height - 1, x + width - 1, y + height - 1, color);
  brights_dbuffer_draw_line(x, y, x, y + height - 1, color);
  brights_dbuffer_draw_line(x + width - 1, y, x + width - 1, y + height - 1, color);
}

void brights_dbuffer_draw_circle(int cx, int cy, int radius, brights_color_t color)
{
  if (radius <= 0) return;
  
  uint32_t pixel = color_to_pixel_fast(color);
  int x = 0, y = radius;
  int d = 3 - 2 * radius;
  
  while (x <= y) {
    dbuffer_set_pixel_fast(cx + x, cy + y, pixel);
    dbuffer_set_pixel_fast(cx - x, cy + y, pixel);
    dbuffer_set_pixel_fast(cx + x, cy - y, pixel);
    dbuffer_set_pixel_fast(cx - x, cy - y, pixel);
    dbuffer_set_pixel_fast(cx + y, cy + x, pixel);
    dbuffer_set_pixel_fast(cx - y, cy + x, pixel);
    dbuffer_set_pixel_fast(cx + y, cy - x, pixel);
    dbuffer_set_pixel_fast(cx - y, cy - x, pixel);
    
    if (d < 0) d += 4 * x++ + 6;
    else d += 4 * (x++ - y--) + 10;
  }
}

void brights_dbuffer_draw_hline(int x, int y, int width, brights_color_t color)
{
  if (!dbuffer.active) { brights_fb_draw_hline(x, y, width, color); return; }
  if ((uint32_t)y >= (uint32_t)dbuffer.height || width <= 0) return;
  
  int cx = x, cw = width;
  if (cx < 0) { cw += cx; cx = 0; }
  int max_w = dbuffer.width - cx;
  if (cw > max_w) cw = max_w;
  if (cw <= 0) return;
  
  uint32_t pixel = color_to_pixel_fast(color);
  uint32_t stride = dbuffer.pitch / 4;
  uint32_t *buf = (uint32_t *)dbuffer.buffer;
  uint32_t *row = buf + y * stride + cx;
  
  for (int i = 0; i < cw; i++) row[i] = pixel;
}

void brights_dbuffer_fill_circle(int cx, int cy, int radius, brights_color_t color)
{
  if (radius <= 0) return;
  
  int x = 0, y = radius;
  int d = 3 - 2 * radius;
  
  while (x <= y) {
    brights_dbuffer_draw_hline(cx - x, cy + y, 2 * x + 1, color);
    if (y != x) brights_dbuffer_draw_hline(cx - x, cy - y, 2 * x + 1, color);
    brights_dbuffer_draw_hline(cx - y, cy + x, 2 * y + 1, color);
    if (y != x) brights_dbuffer_draw_hline(cx - y, cy - x, 2 * y + 1, color);
    
    if (d < 0) d += 4 * x++ + 6;
    else d += 4 * (x++ - y--) + 10;
  }
}

void brights_dbuffer_blit(void *buffer, int x, int y, int width, int height, int pitch)
{
  if (!dbuffer.active) { brights_fb_blit(buffer, x, y, width, height, pitch); return; }
  if (!buffer) return;
  if ((uint32_t)x >= (uint32_t)dbuffer.width || (uint32_t)y >= (uint32_t)dbuffer.height) return;
  
  uint32_t *dst = (uint32_t *)dbuffer.buffer;
  uint32_t *src = (uint32_t *)buffer;
  uint32_t dst_stride = dbuffer.pitch / 4;
  uint32_t src_stride = (uint32_t)pitch / 4;
  
  for (int i = 0; i < height; i++) {
    uint32_t *dst_row = dst + (y + i) * dst_stride + x;
    uint32_t *src_row = src + i * src_stride;
    for (int j = 0; j < width; j++) dst_row[j] = src_row[j];
  }
}

void brights_dbuffer_fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2, brights_color_t color)
{
  if (y0 > y1) { int t = y0; y0 = y1; y1 = t; t = x0; x0 = x1; x1 = t; }
  if (y0 > y2) { int t = y0; y0 = y2; y2 = t; t = x0; x0 = x2; x2 = t; }
  if (y1 > y2) { int t = y1; y1 = y2; y2 = t; t = x1; x1 = x2; x2 = t; }
  
  int total_height = y2 - y0;
  if (total_height <= 0) return;
  
  for (int y = y0; y <= y2; y++) {
    float seg_start, seg_end;
    
    if (y < y1) {
      float t1 = (float)(y - y0) / (y1 - y0);
      float t2 = (float)(y - y0) / (y2 - y0);
      seg_start = x0 + t1 * (x1 - x0);
      seg_end = x0 + t2 * (x2 - x0);
    } else {
      float t1 = (float)(y - y1) / (y2 - y1);
      float t2 = (float)(y - y0) / (y2 - y0);
      seg_start = x1 + t1 * (x2 - x1);
      seg_end = x0 + t2 * (x2 - x0);
    }
    
    int xs = (int)(seg_start < seg_end ? seg_start : seg_end);
    int xe = (int)(seg_start < seg_end ? seg_end : seg_start);
    
    if (xs < 0) xs = 0;
    int max_x = dbuffer.width;
    if (xe > max_x) xe = max_x;
    
    if (xe > xs) brights_dbuffer_draw_hline(xs, y, xe - xs, color);
  }
}

void brights_clip_set(int x, int y, int width, int height)
{
  global_clip.x = x;
  global_clip.y = y;
  global_clip.width = width;
  global_clip.height = height;
  global_clip.enabled = 1;
}

void brights_clip_disable(void) { global_clip.enabled = 0; }
brights_clip_t *brights_clip_get(void) { return &global_clip; }

int brights_clip_point(int x, int y)
{
  if (!global_clip.enabled) return 1;
  return (x >= global_clip.x && x < global_clip.x + global_clip.width &&
          y >= global_clip.y && y < global_clip.y + global_clip.height);
}

int brights_clip_rect(int x, int y, int width, int height)
{
  if (!global_clip.enabled) return 1;
  return (x < global_clip.x + global_clip.width &&
          x + width > global_clip.x &&
          y < global_clip.y + global_clip.height &&
          y + height > global_clip.y);
}

void brights_gc_init(brights_gc_t *gc)
{
  if (!gc) return;
  gc->foreground = (brights_color_t){255, 255, 255, 255};
  gc->background = (brights_color_t){0, 0, 0, 255};
  gc->thickness = 1;
  gc->double_buffer = 0;
  gc->vsync = 0;
  brights_clip_disable();
}

void brights_gc_set_fg(brights_gc_t *gc, brights_color_t color) { if (gc) gc->foreground = color; }
void brights_gc_set_bg(brights_gc_t *gc, brights_color_t color) { if (gc) gc->background = color; }
void brights_gc_set_thickness(brights_gc_t *gc, int thickness) { if (gc && thickness > 0) gc->thickness = thickness; }
void brights_gc_set_clip(brights_gc_t *gc, int x, int y, int width, int height) { if (gc) { brights_clip_set(x, y, width, height); gc->clip = global_clip; } }
void brights_gc_enable_double_buffer(brights_gc_t *gc) { if (gc) { gc->double_buffer = 1; brights_dbuffer_init(); } }
void brights_gc_disable_double_buffer(brights_gc_t *gc) { if (gc) gc->double_buffer = 0; }

brights_color_t brights_rgb(uint8_t r, uint8_t g, uint8_t b) { return (brights_color_t){r, g, b, 255}; }
brights_color_t brights_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) { return (brights_color_t){r, g, b, a}; }

brights_color_t brights_color_from_hex(uint32_t hex)
{
  return (brights_color_t){(uint8_t)(hex >> 16), (uint8_t)(hex >> 8), (uint8_t)hex, 255};
}

uint32_t brights_color_to_hex(brights_color_t color)
{
  return ((uint32_t)color.r << 16) | ((uint32_t)color.g << 8) | color.b;
}

brights_color_t brights_color_lighten(brights_color_t color, float factor)
{
  if (factor < 0) factor = 0;
  if (factor > 1) factor = 1;
  float inv = 1.0f - factor;
  color.r = (uint8_t)(color.r * inv + 255 * factor);
  color.g = (uint8_t)(color.g * inv + 255 * factor);
  color.b = (uint8_t)(color.b * inv + 255 * factor);
  return color;
}

brights_color_t brights_color_darken(brights_color_t color, float factor)
{
  if (factor < 0) factor = 0;
  if (factor > 1) factor = 1;
  color.r = (uint8_t)(color.r * (1.0f - factor));
  color.g = (uint8_t)(color.g * (1.0f - factor));
  color.b = (uint8_t)(color.b * (1.0f - factor));
  return color;
}

int brights_color_equal(brights_color_t c1, brights_color_t c2)
{
  return c1.r == c2.r && c1.g == c2.g && c1.b == c2.b && c1.a == c2.a;
}

void brights_fb_draw_line_aa(int x0, int y0, int x1, int y1, brights_color_t color)
{
  int dx = x1 - x0, dy = y1 - y0;
  int abs_dx = dx >= 0 ? dx : -dx;
  int abs_dy = dy >= 0 ? dy : -dy;
  int xinc = dx > 0 ? 1 : -1;
  int yinc = dy > 0 ? 1 : -1;
  uint32_t pixel = color_to_pixel_fast(color);
  
  if (abs_dx > abs_dy) {
    int e = abs_dx / 2;
    for (int i = 0; i < abs_dx; i++) {
      set_pixel_fast(x0, y0, pixel);
      x0 += xinc; e -= abs_dy;
      if (e < 0) { y0 += yinc; e += abs_dx; }
    }
  } else if (abs_dy > 0) {
    int e = abs_dy / 2;
    for (int i = 0; i < abs_dy; i++) {
      set_pixel_fast(x0, y0, pixel);
      y0 += yinc; e -= abs_dx;
      if (e < 0) { x0 += xinc; e += abs_dy; }
    }
  }
  set_pixel_fast(x1, y1, pixel);
}

void brights_fb_fill_gradient_h(int x, int y, int width, int height, brights_color_t c1, brights_color_t color2)
{
  if (width <= 0 || height <= 0) return;
  
  for (int i = 0; i < width; i++) {
    uint32_t t = (i * 255) / (width - 1);
    uint32_t inv = 255 - t;
    brights_color_t color = {
      (uint8_t)((c1.r * inv + color2.r * t + 127) / 255),
      (uint8_t)((c1.g * inv + color2.g * t + 127) / 255),
      (uint8_t)((c1.b * inv + color2.b * t + 127) / 255),
      255
    };
    brights_fb_fill_rect(x + i, y, 1, height, color);
  }
}

void brights_fb_fill_gradient_v(int x, int y, int width, int height, brights_color_t c1, brights_color_t color2)
{
  if (width <= 0 || height <= 0) return;
  
  for (int i = 0; i < height; i++) {
    uint32_t t = (i * 255) / (height - 1);
    uint32_t inv = 255 - t;
    brights_color_t color = {
      (uint8_t)((c1.r * inv + color2.r * t + 127) / 255),
      (uint8_t)((c1.g * inv + color2.g * t + 127) / 255),
      (uint8_t)((c1.b * inv + color2.b * t + 127) / 255),
      255
    };
    brights_fb_fill_rect(x, y + i, width, 1, color);
  }
}

void brights_fb_scroll(int dx, int dy)
{
  if (dx == 0 && dy == 0) return;
  
  uint32_t *fb = (uint32_t *)fb_info.framebuffer;
  uint32_t stride = fb_info.pitch / 4;
  uint32_t width = fb_info.width;
  uint32_t height = fb_info.height;
  
  if (dy > 0 && (uint32_t)dy < height) {
    uint32_t rows = height - dy;
    for (uint32_t y = 0; y < rows; y++) {
      uint32_t *src_row = fb + (y + dy) * stride;
      uint32_t *dst_row = fb + y * stride;
      for (uint32_t x = 0; x < width; x++) dst_row[x] = src_row[x];
    }
  } else if (dy < 0 && (uint32_t)(-dy) < height) {
    uint32_t rows = height + dy;
    for (uint32_t y = rows; y > 0; y--) {
      uint32_t *src_row = fb + (y + dy) * stride;
      uint32_t *dst_row = fb + y * stride;
      for (uint32_t x = 0; x < width; x++) dst_row[x] = src_row[x];
    }
  }
  
  if (dx > 0 && (uint32_t)dx < width) {
    uint32_t cols = width - dx;
    for (uint32_t y = 0; y < height; y++) {
      for (uint32_t x = cols; x > 0; x--) {
        fb[y * stride + x + dx - 1] = fb[y * stride + x - 1];
      }
    }
  } else if (dx < 0 && (uint32_t)(-dx) < width) {
    uint32_t cols = width + dx;
    for (uint32_t y = 0; y < height; y++) {
      for (uint32_t x = 0; x < cols; x++) {
        fb[y * stride + x] = fb[y * stride + x - dx];
      }
    }
  }
}

void brights_fb_scroll_area(int x, int y, int width, int height, int dx, int dy)
{
  if (width <= 0 || height <= 0) return;
  if (x < 0 || y < 0 || x + width > (int)fb_info.width || y + height > (int)fb_info.height) return;
  
  brights_clip_set(x, y, width, height);
  brights_fb_scroll(dx, dy);
  brights_clip_disable();
}
