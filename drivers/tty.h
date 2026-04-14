#ifndef BRIGHTS_TTY_H
#define BRIGHTS_TTY_H

#include <stdint.h>

/* TTY modes */
#define TTY_MODE_RAW    0
#define TTY_MODE_COOKED 1

void brights_tty_init(void);
void brights_tty_write(const uint16_t *s);
void brights_tty_write_str(const char *s);
void brights_tty_write_char(char ch);
int brights_tty_read_char(char *out_ch);
char brights_tty_read_char_blocking(void);

/* Set/get TTY mode */
void brights_tty_set_mode(int mode);
int brights_tty_get_mode(void);

#endif
