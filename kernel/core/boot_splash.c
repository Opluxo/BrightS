#include "boot_splash.h"
#include "../drivers/serial.h"
#include "../drivers/fb.h"
#include "../drivers/font.h"
#include "sleep.h"

#define BOX_PAD 12
#define ART_PAD 16
#define VERT_PAD 6

static void print_spaces(int n)
{
  for (int i = 0; i < n; ++i)
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, " ");
}

static void logo_line_centered(const char *line)
{
  print_spaces(BOX_PAD);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, line);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\r\n");
}

static void art_line(const char *line)
{
  print_spaces(ART_PAD);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, line);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\r\n");
}

static inline int str_len(const char *s)
{
  int n = 0;
  while (s[n]) ++n;
  return n;
}

static void fb_draw_splash(void)
{
  brights_fb_info_t *info = brights_fb_get_info();
  if (!info || !info->initialized) return;

  int fb_w = (int)info->width;
  int fb_h = (int)info->height;

  /* Dark gradient background: top (10,15,40) -> bottom (5,5,20) */
  brights_fb_fill_gradient_v(0, 0, fb_w, fb_h,
    brights_rgb(10, 15, 40), brights_rgb(5, 5, 20));

  int char_w = 8, char_h = 16;

  /* Logo dimensions: 5 rows x 48 chars = 384px wide, 5 rows = 80px tall */
  int logo_w = 48 * char_w;
  int logo_h = 5 * char_h;

  /* Box dimensions: logo + padding */
  int box_w = logo_w + 64;
  int box_h = logo_h + 8 * char_h + 32;

  int box_x = (fb_w - box_w) / 2;
  int box_y = (fb_h - box_h) / 2;
  if (box_x < 10) box_x = 10;
  if (box_y < 10) box_y = 10;

  /* Shadow (offset 4px) */
  brights_fb_fill_rounded_rect(box_x + 4, box_y + 4, box_w, box_h, 8,
    brights_rgb(0, 0, 0));

  /* Box background with slight transparency effect (solid dark) */
  brights_fb_fill_rounded_rect(box_x, box_y, box_w, box_h, 8,
    brights_rgb(12, 18, 45));

  /* Box border (cyan) */
  brights_fb_draw_rounded_rect(box_x, box_y, box_w, box_h, 8,
    brights_rgb(0, 180, 220));

  /* Inner accent line (top) */
  brights_fb_draw_hline(box_x + 8, box_y + 1, box_w - 16,
    brights_rgb(0, 220, 255));

  /* Draw ASCII art logo centered in box */
  const char *logo[] = {
    "#####  #####  #####  #####  #   #  #####  ##### ",
    "#   #  #   #    #    #      #   #    #    #     ",
    "#####  #####    #    #  ##  #####    #    ##### ",
    "#   #  # #      #    #   #  #   #    #        # ",
    "#   #  #  #   #####  #####  #   #    #    ##### ",
  };

  int logo_x = box_x + (box_w - logo_w) / 2;
  int logo_y = box_y + 24;

  uint32_t cyan_px = (0 << 16) | (240 << 8) | 255;
  uint32_t transparent = 0xFFFFFFFF;

  for (int row = 0; row < 5; ++row) {
    for (int col = 0; col < 48; ++col) {
      if (logo[row][col] == '#') {
        brights_font_draw_char(logo_x + col * char_w, logo_y + row * char_h,
          '#', cyan_px, transparent);
      }
    }
  }

  /* Separator line below logo */
  int sep_y = logo_y + logo_h + 12;
  brights_fb_draw_hline(box_x + 32, sep_y, box_w - 64,
    brights_rgb(0, 120, 160));

  /* Version text */
  int text_y = sep_y + 16;
  const char *ver = "BrightS v0.1.3";
  int ver_w = str_len(ver) * char_w;
  brights_font_draw_string(box_x + (box_w - ver_w) / 2, text_y,
    ver, (255 << 16) | (255 << 8) | 255, transparent);

  /* Author text */
  int auth_y = text_y + char_h + 8;
  const char *auth = "Designed by Opluxo LLC";
  int auth_w = str_len(auth) * char_w;
  brights_font_draw_string(box_x + (box_w - auth_w) / 2, auth_y,
    auth, (120 << 16) | (180 << 8) | 220, transparent);

  /* Status message (green, below box) */
  const char *status = "System initialization complete. Starting login...";
  int status_w = str_len(status) * char_w;
  int status_y = box_y + box_h + 24;
  if (status_y + char_h > fb_h) status_y = box_y + box_h - char_h - 8;
  brights_font_draw_string((fb_w - status_w) / 2, status_y,
    status, (0 << 16) | (220 << 8) | 100, transparent);

  /* Flush double buffer to screen */
  brights_dbuffer_flip();
}

void brights_boot_splash(void)
{
  fb_draw_splash();

  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\033[2J");
  for (int i = 0; i < VERT_PAD; ++i)
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\r\n");

  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\033[1;36m");
  art_line("#####  #####  #####  #####  #   #  #####  ##### ");
  art_line("#   #  #   #    #    #      #   #    #    #     ");
  art_line("#####  #####    #    #  ##  #####    #    ##### ");
  art_line("#   #  # #      #    #   #  #   #    #        # ");
  art_line("#   #  #  #   #####  #####  #   #    #    ##### ");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\033[0m\r\n");

  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\033[1;36m");
  logo_line_centered("+------------------------------------------------------+");
  logo_line_centered("|                 BrightS v0.1.3                     |");
  logo_line_centered("|              Designed by Opluxo LLC                  |");
  logo_line_centered("+------------------------------------------------------+");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\033[0m\r\n");

  print_spaces(BOX_PAD);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\033[1;32m");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "System initialization complete. Starting login...");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\033[0m\r\n");

  brights_sleep_ms(1000);
}
