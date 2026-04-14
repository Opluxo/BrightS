#include "fb.h"
#include "../include/kernel/uefi.h"
#include <stdint.h>

static brights_fb_info_t fb_info = {0};

int brights_fb_init(void *gop_ptr)
{
  if (!gop_ptr) return -1;
  
  EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = (EFI_GRAPHICS_OUTPUT_PROTOCOL *)gop_ptr;
  
  if (!gop || !gop->Mode) return -1;
  
  EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *mode = gop->Mode;
  
  fb_info.framebuffer = (void *)(uintptr_t)mode->FrameBufferBase;
  fb_info.width = mode->Info->HorizontalResolution;
  fb_info.height = mode->Info->VerticalResolution;
  fb_info.pitch = mode->Info->PixelsPerScanLine;
  fb_info.bpp = mode->Info->PixelsPerScanLine * 8 / mode->Info->HorizontalResolution;
  fb_info.bytes_per_pixel = mode->Info->PixelsPerScanLine / mode->Info->HorizontalResolution;
  
  /* Calculate bytes per pixel from format */
  switch (mode->Info->PixelFormat) {
    case PixelRedGreenBlueReserved8BitPerColor:
    case PixelBlueGreenRedReserved8BitPerColor:
      fb_info.bpp = 32;
      fb_info.bytes_per_pixel = 4;
      fb_info.red_mask = 0x00FF0000;
      fb_info.green_mask = 0x0000FF00;
      fb_info.blue_mask = 0x000000FF;
      fb_info.red_shift = 16;
      fb_info.green_shift = 8;
      fb_info.blue_shift = 0;
      break;
    case PixelBitMask:
      fb_info.red_mask = mode->Info->PixelInformation.RedMask;
      fb_info.green_mask = mode->Info->PixelInformation.GreenMask;
      fb_info.blue_mask = mode->Info->PixelInformation.BlueMask;
      /* Calculate shifts */
      fb_info.red_shift = 0;
      while ((fb_info.red_mask & (1 << fb_info.red_shift)) == 0 && fb_info.red_shift < 32) fb_info.red_shift++;
      fb_info.green_shift = 0;
      while ((fb_info.green_mask & (1 << fb_info.green_shift)) == 0 && fb_info.green_shift < 32) fb_info.green_shift++;
      fb_info.blue_shift = 0;
      while ((fb_info.blue_mask & (1 << fb_info.blue_shift)) == 0 && fb_info.blue_shift < 32) fb_info.blue_shift++;
      /* Calculate BPP from masks */
      fb_info.bpp = 0;
      if (fb_info.red_mask) fb_info.bpp = fb_info.red_shift + 8;
      if (fb_info.green_mask) fb_info.bpp = fb_info.green_shift + 8;
      if (fb_info.blue_mask) {
        int shift = fb_info.blue_shift + 8;
        if (shift > fb_info.bpp) fb_info.bpp = shift;
      }
      fb_info.bytes_per_pixel = (fb_info.bpp + 7) / 8;
      break;
    default:
      fb_info.bpp = 32;
      fb_info.bytes_per_pixel = 4;
      fb_info.red_mask = 0x00FF0000;
      fb_info.green_mask = 0x0000FF00;
      fb_info.blue_mask = 0x000000FF;
      fb_info.red_shift = 16;
      fb_info.green_shift = 8;
      fb_info.blue_shift = 0;
      break;
  }
  
  fb_info.initialized = 1;
  
  return 0;
}

int brights_fb_available(void)
{
  return fb_info.initialized;
}

brights_fb_info_t *brights_fb_get_info(void)
{
  if (!fb_info.initialized) return 0;
  return &fb_info;
}

static uint32_t color_to_pixel(brights_color_t color)
{
  uint32_t r = color.r;
  uint32_t g = color.g;
  uint32_t b = color.b;
  
  return ((r >> (8 - (32 - fb_info.red_shift - 8))) & (0xFF >> (8 - (32 - fb_info.red_shift - 8)))) |
         ((g >> (8 - (32 - fb_info.green_shift - 8))) & (0xFF >> (8 - (32 - fb_info.green_shift - 8)))) |
         ((b >> (8 - (32 - fb_info.blue_shift - 8))) & (0xFF >> (8 - (32 - fb_info.blue_shift - 8))));
}

void brights_fb_draw_pixel(int x, int y, brights_color_t color)
{
  if (!fb_info.initialized) return;
  if (x < 0 || x >= (int)fb_info.width || y < 0 || y >= (int)fb_info.height) return;
  
  uint32_t *fb = (uint32_t *)fb_info.framebuffer;
  uint32_t offset = y * (fb_info.pitch / 4) + x;
  
  uint32_t pixel = (color.r << fb_info.red_shift) |
                   (color.g << fb_info.green_shift) |
                   (color.b << fb_info.blue_shift);
  fb[offset] = pixel;
}

void brights_fb_draw_hline(int x, int y, int width, brights_color_t color)
{
  if (!fb_info.initialized) return;
  if (y < 0 || y >= (int)fb_info.height) return;
  if (x < 0) { width += x; x = 0; }
  if (x + width > (int)fb_info.width) width = fb_info.width - x;
  if (width <= 0) return;
  
  uint32_t *fb = (uint32_t *)fb_info.framebuffer;
  uint32_t pixel = (color.r << fb_info.red_shift) |
                   (color.g << fb_info.green_shift) |
                   (color.b << fb_info.blue_shift);
  
  uint32_t offset = y * (fb_info.pitch / 4) + x;
  for (int i = 0; i < width; i++) {
    fb[offset + i] = pixel;
  }
}

void brights_fb_draw_vline(int x, int y, int height, brights_color_t color)
{
  if (!fb_info.initialized) return;
  if (x < 0 || x >= (int)fb_info.width) return;
  if (y < 0) { height += y; y = 0; }
  if (y + height > (int)fb_info.height) height = fb_info.height - y;
  if (height <= 0) return;
  
  uint32_t *fb = (uint32_t *)fb_info.framebuffer;
  uint32_t pixel = (color.r << fb_info.red_shift) |
                   (color.g << fb_info.green_shift) |
                   (color.b << fb_info.blue_shift);
  
  uint32_t stride = fb_info.pitch / 4;
  uint32_t offset = y * stride + x;
  for (int i = 0; i < height; i++) {
    fb[offset + i * stride] = pixel;
  }
}

void brights_fb_draw_rect(int x, int y, int width, int height, brights_color_t color)
{
  if (!fb_info.initialized) return;
  brights_fb_draw_hline(x, y, width, color);
  brights_fb_draw_hline(x, y + height - 1, width, color);
  brights_fb_draw_vline(x, y, height, color);
  brights_fb_draw_vline(x + width - 1, y, height, color);
}

void brights_fb_fill_rect(int x, int y, int width, int height, brights_color_t color)
{
  if (!fb_info.initialized) return;
  if (x < 0) { width += x; x = 0; }
  if (y < 0) { height += y; y = 0; }
  if (x + width > (int)fb_info.width) width = fb_info.width - x;
  if (y + height > (int)fb_info.height) height = fb_info.height - y;
  if (width <= 0 || height <= 0) return;
  
  uint32_t *fb = (uint32_t *)fb_info.framebuffer;
  uint32_t pixel = (color.r << fb_info.red_shift) |
                   (color.g << fb_info.green_shift) |
                   (color.b << fb_info.blue_shift);
  
  uint32_t stride = fb_info.pitch / 4;
  uint32_t offset = y * stride + x;
  
  for (int row = 0; row < height; row++) {
    uint32_t row_offset = offset + row * stride;
    for (int col = 0; col < width; col++) {
      fb[row_offset + col] = pixel;
    }
  }
}

void brights_fb_blit(void *buffer, int x, int y, int width, int height, int pitch)
{
  if (!fb_info.initialized || !buffer) return;
  if (x < 0 || x + width > (int)fb_info.width) return;
  if (y < 0 || y + height > (int)fb_info.height) return;
  
  uint32_t *fb = (uint32_t *)fb_info.framebuffer;
  uint32_t *src = (uint32_t *)buffer;
  
  uint32_t fb_stride = fb_info.pitch / 4;
  uint32_t src_stride = pitch / 4;
  
  for (int row = 0; row < height; row++) {
    uint32_t fb_offset = (y + row) * fb_stride + x;
    uint32_t src_offset = row * src_stride;
    for (int col = 0; col < width; col++) {
      fb[fb_offset + col] = src[src_offset + col];
    }
  }
}

brights_color_t brights_fb_get_pixel(int x, int y)
{
  brights_color_t color = {0, 0, 0, 255};
  if (!fb_info.initialized) return color;
  if (x < 0 || x >= (int)fb_info.width || y < 0 || y >= (int)fb_info.height) return color;
  
  uint32_t *fb = (uint32_t *)fb_info.framebuffer;
  uint32_t offset = y * (fb_info.pitch / 4) + x;
  uint32_t pixel = fb[offset];
  
  color.r = (pixel >> fb_info.red_shift) & 0xFF;
  color.g = (pixel >> fb_info.green_shift) & 0xFF;
  color.b = (pixel >> fb_info.blue_shift) & 0xFF;
  
  return color;
}

void brights_fb_fill(brights_color_t color)
{
  brights_fb_fill_rect(0, 0, fb_info.width, fb_info.height, color);
}

void brights_fb_clear(brights_color_t color)
{
  brights_fb_fill(color);
}
