#include "tty.h"
#include "serial.h"
#include "ps2kbd.h"
#include "../core/printf.h"

static brights_console_t tty_console;
static int tty_ready;
static int tty_mode = TTY_MODE_COOKED;

/* Line buffer for cooked mode */
#define TTY_LINE_BUF 256
static char line_buf[TTY_LINE_BUF];
static int line_len = 0;
static int line_ready = 0;
static int line_pos = 0;

void brights_tty_init(void)
{
  if (!tty_ready) {
    brights_serial_console_init(&tty_console, BRIGHTS_COM1_PORT);
    tty_ready = 1;
  }
}

void brights_tty_set_mode(int mode)
{
  tty_mode = mode;
}

int brights_tty_get_mode(void)
{
  return tty_mode;
}

void brights_tty_write(const uint16_t *s)
{
  if (!s) return;
  brights_tty_init();
  brights_print(&tty_console, s);
}

void brights_tty_write_str(const char *s)
{
  if (!s) return;
  brights_tty_init();
  while (*s) {
    char ch[2] = {*s, 0};
    brights_serial_write_ascii(BRIGHTS_COM1_PORT, ch);
    ++s;
  }
}

void brights_tty_write_char(char ch)
{
  char s[2] = {ch, 0};
  brights_serial_write_ascii(BRIGHTS_COM1_PORT, s);
}

/* Process a raw character in cooked mode */
static void cooked_process_char(char ch)
{
  if (ch == '\b' || ch == 0x7F) {
    /* Backspace */
    if (line_len > 0) {
      --line_len;
      /* Echo: backspace, space, backspace */
      brights_tty_write_char('\b');
      brights_tty_write_char(' ');
      brights_tty_write_char('\b');
    }
    return;
  }

  if (ch == '\n' || ch == '\r') {
    /* Line complete */
    line_buf[line_len] = '\n';
    line_len++;
    line_buf[line_len] = 0;
    line_ready = 1;
    line_pos = 0;
    brights_tty_write_char('\n');
    return;
  }

  if (ch == 27) {
    /* ESC - discard line */
    line_len = 0;
    line_buf[0] = 0;
    brights_tty_write_char('\n');
    return;
  }

  if (ch == '\t') {
    /* Tab → spaces */
    for (int i = 0; i < 4 && line_len < TTY_LINE_BUF - 2; ++i) {
      line_buf[line_len++] = ' ';
      brights_tty_write_char(' ');
    }
    return;
  }

  /* Regular character */
  if (line_len < TTY_LINE_BUF - 2) {
    line_buf[line_len++] = ch;
    brights_tty_write_char(ch);
  }
}

int brights_tty_read_char(char *out_ch)
{
  if (!out_ch) return -1;
  brights_tty_init();

  if (tty_mode == TTY_MODE_COOKED) {
    /* If we have a completed line, serve from it */
    if (line_ready && line_pos < line_len) {
      *out_ch = line_buf[line_pos++];
      if (line_pos >= line_len) {
        line_ready = 0;
        line_len = 0;
        line_pos = 0;
      }
      return 1;
    }

    /* Try to get a raw character and process it */
    char raw_ch;
    if (brights_ps2kbd_read_char(&raw_ch) > 0) {
      cooked_process_char(raw_ch);
      /* After processing, check if line is ready */
      if (line_ready && line_pos < line_len) {
        *out_ch = line_buf[line_pos++];
        if (line_pos >= line_len) {
          line_ready = 0;
          line_len = 0;
          line_pos = 0;
        }
        return 1;
      }
    }

    /* Also check serial input */
    uint8_t serial_ch;
    if (brights_serial_read_byte(BRIGHTS_COM1_PORT, &serial_ch) > 0) {
      cooked_process_char((char)serial_ch);
      if (line_ready && line_pos < line_len) {
        *out_ch = line_buf[line_pos++];
        if (line_pos >= line_len) {
          line_ready = 0;
          line_len = 0;
          line_pos = 0;
        }
        return 1;
      }
    }

    return 0;
  }

  /* Raw mode */
  if (brights_ps2kbd_read_char(out_ch) > 0) return 1;
  uint8_t serial_ch;
  if (brights_serial_read_byte(BRIGHTS_COM1_PORT, &serial_ch) > 0) {
    *out_ch = (char)serial_ch;
    return 1;
  }
  return 0;
}

char brights_tty_read_char_blocking(void)
{
  char ch = 0;
  while (brights_tty_read_char(&ch) <= 0) {
    __asm__ __volatile__("hlt");
  }
  return ch;
}
