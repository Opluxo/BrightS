#include "boot_splash.h"
#include "../drivers/serial.h"
#include "sleep.h"

/* Box is 56 chars wide, terminal is 80 cols, so center pad = (80-56)/2 = 12 */
#define BOX_PAD 12
/* Art banner is 48 chars wide, (80-48)/2 = 16 */
#define ART_PAD 16
/*
 * 24-row terminal content:
 *   5 art + 1 blank + 4 box + 1 blank + 1 status = 12 rows
 *   VERT_PAD = (24 - 12) / 2 = 6
 */
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

void brights_boot_splash(void)
{
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
  logo_line_centered("|                 BrightS 0.1.2.2                      |");
  logo_line_centered("|            Designed by OpenLight Studio              |");
  logo_line_centered("+------------------------------------------------------+");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\033[0m\r\n");

  print_spaces(BOX_PAD);
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\033[1;32m");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "System initialization complete. Starting login...");
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, "\033[0m\r\n");

  brights_sleep_ms(1000);
}
