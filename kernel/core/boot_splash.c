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

  brights_color_t bg_struct = {0, 0, 40, 255};
  uint32_t bg = ((uint32_t)bg_struct.r << 16) | ((uint32_t)bg_struct.g << 8) | (uint32_t)bg_struct.b;
  uint32_t cyan = (0 << 16) | (255 << 8) | 255;
  uint32_t green = (0 << 16) | (255 << 8) | 0;

  brights_fb_clear(bg_struct);

  int char_w = 8, char_h = 16;
  int logo_start_y = (fb_h - 12 * char_h) / 2;
  if (logo_start_y < 0) logo_start_y = 20;

  const char *logo[] = {
    "#####  #####  #####  #####  #   #  #####  ##### ",
    "#   #  #   #    #    #      #   #    #    #     ",
    "#####  #####    #    #  ##  #####    #    ##### ",
    "#   #  # #      #    #   #  #   #    #        # ",
    "#   #  #  #   #####  #####  #   #    #    ##### ",
  };

  int logo_width = 48;
  int logo_x = (fb_w - logo_width * char_w) / 2;

  for (int row = 0; row < 5; ++row) {
    for (int col = 0; col < logo_width; ++col) {
      if (logo[row][col] == '#') {
        brights_font_draw_char(logo_x + col * char_w, logo_start_y + row * char_h,
                               '#', cyan, bg);
      }
    }
  }

  int text_y = logo_start_y + 6 * char_h;
  int box_start_x = (fb_w - 56 * char_w) / 2;

  brights_font_draw_string(box_start_x, text_y,
    "|                 BrightS 0.1.2.4                      |", cyan, bg);
  brights_font_draw_string(box_start_x, text_y + char_h,
    "|            Designed by OpenLight Studio              |", cyan, bg);

  const char *status = "System initialization complete. Starting login...";
  int status_x = (fb_w - str_len(status) * char_w) / 2;
  brights_font_draw_string(status_x, text_y + 3 * char_h, status, green, bg);
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
  logo_line_centered("|                 BrightS 0.1.2.4                      |");
  logo_line_centered("|            Designed by OpenLight Studio              |");
  logo_line_centered("+------------------------------------------------------+");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\033[0m\r\n");

  print_spaces(BOX_PAD);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\033[1;32m");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "System initialization complete. Starting login...");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\033[0m\r\n");

  brights_sleep_ms(1000);
}
